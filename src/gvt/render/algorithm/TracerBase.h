/*
 * Tracer.h
 *
 *  Created on: Nov 27, 2013
 *      Author: jbarbosa
 */

#ifndef GVT_RENDER_ALGORITHM_TRACER_BASE_H
#define GVT_RENDER_ALGORITHM_TRACER_BASE_H

#include <gvt/core/Debug.h>
#include <gvt/core/schedule/TaskScheduling.h>
#include <gvt/render/RenderContext.h>
#include <gvt/render/data/Primitives.h>
#include <gvt/render/data/scene/ColorAccumulator.h>
#include <gvt/render/data/scene/Image.h>

#include <boost/foreach.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/timer/timer.hpp>

#include <algorithm>
#include <future>
#include <numeric>

#include <mpi.h>

#include <map>

namespace gvt {
namespace render {
namespace algorithm {

/// Tracer base class

struct GVT_COMM {
  size_t rank;
  size_t world_size;

  GVT_COMM() {
    rank = MPI::COMM_WORLD.Get_rank();
    world_size = MPI::COMM_WORLD.Get_size();
  }

  operator bool() { return (world_size > 1); }
  bool root() { return rank == 0; }

};

struct processRay;

class AbstractTrace {
 public:
  ///! Define mpi communication world
  GVT_COMM mpi;

  gvt::render::actor::RayVector& rays;     ///< Rays to trace
  gvt::render::data::scene::Image& image;  ///< Final image buffer
  gvt::render::RenderContext& cntxt = *gvt::render::RenderContext::instance();
  //gvt::core::CoreContext& cntxt = *gvt::render::RenderContext::instance();
  gvt::core::DBNodeH rootnode = cntxt.getRootNode();
  gvt::render::data::Dataset* data = gvt::core::variant_toDatasetPointer(rootnode["Dataset"]["Dataset_Pointer"].value());
  int width = gvt::core::variant_toInteger(rootnode["Film"]["width"].value());
  int height = gvt::core::variant_toInteger(rootnode["Film"]["height"].value());

  // never used BDS
  //unsigned char* vtf;
  float sample_ratio;

  boost::mutex raymutex;
  boost::mutex* queue_mutex;
  std::map<int, gvt::render::actor::RayVector> queue;  ///< Node rays working
  /// queue
  // std::map<int, std::mutex> queue;
  // buffer for color accumulation
  boost::mutex* colorBuf_mutex;
  GVT_COLOR_ACCUM* colorBuf;

  AbstractTrace(gvt::render::actor::RayVector& rays,
                gvt::render::data::scene::Image& image)
      : rays(rays), image(image) {
    // never used BDS
    //vtf = gvt::render::Attributes::rta->GetTransferFunction();
    // never used BDS
    //sample_ratio = gvt::render::Attributes::rta->sample_ratio;
    colorBuf = new GVT_COLOR_ACCUM[width*height];
    queue_mutex = new boost::mutex[data->size()];
    colorBuf_mutex = new boost::mutex[width];
    //colorBuf = new GVT_COLOR_ACCUM[gvt::render::RTA::instance()->view.width *
     //                              gvt::render::RTA::instance()->view.height];
    //queue_mutex = new boost::mutex[gvt::render::Attributes::rta->dataset->size()];
    //colorBuf_mutex = new boost::mutex[gvt::render::RTA::instance()->view.width];

  }

  virtual ~AbstractTrace() {
  };
  virtual void operator()(void) {
    GVT_ASSERT_BACKTRACE(0, "Not supported");
  };

  virtual void FilterRaysLocally(void) {
    GVT_DEBUG(DBG_ALWAYS, "Generate rays filtering : " << rays.size());
    shuffleRays(rays);
  }

  /***
  *   Given a queue of rays:
  *     - Moves the ray to the next domain on the list
  *     -
  *
  */

  virtual void shuffleRays(
      gvt::render::actor::RayVector& rays,
      gvt::render::data::domain::AbstractDomain* dom = NULL) {

    GVT_DEBUG(DBG_ALWAYS,"["<< mpi.rank << "] Shuffle");
    GVT_DEBUG(DBG_ALWAYS,"["<< mpi.rank << "] Shuffle rays.size() " << rays.size() << std::endl);
    boost::timer::auto_cpu_timer t("Ray shuflle %t\n");
    int nchunks = 1;  // std::thread::hardware_concurrency();
    int chunk_size = rays.size() / nchunks;
    std::vector< std::pair<int, int> > chunks;
    std::vector< std::future<void> > futures;
    for (int ii = 0; ii < nchunks - 1; ii++) {
      chunks.push_back(
          std::make_pair(ii * chunk_size, ii * chunk_size + chunk_size));
    }
    int ii = nchunks - 1;
    chunks.push_back(std::make_pair(ii * chunk_size, rays.size()));
    for (auto limit : chunks) {
      // futures.push_back(std::async(std::launch::deferred, [&]() {
        int chunk = limit.second - limit.first;
        std::map<int, gvt::render::actor::RayVector> local_queue;
        gvt::render::actor::RayVector local(chunk);
        local.assign(rays.begin() + limit.first, rays.begin() + limit.second);
        for (gvt::render::actor::Ray& r : local) {
          gvt::render::actor::isecDomList& len2List = r.domains;

          if (len2List.empty() && dom) dom->marchOut(r);

          if (len2List.empty()) {
            //gvt::render::Attributes::rta->dataset->intersect(r, len2List);
            data->intersect(r,len2List);
          }

          if (!len2List.empty()) {
            int firstDomainOnList = (*len2List.begin());
            len2List.erase(len2List.begin());
            local_queue[firstDomainOnList].push_back(r);

          } else if (dom) {
            boost::mutex::scoped_lock fbloc(
                colorBuf_mutex
                    [r.id % width]);
                    //[r.id % gvt::render::Attributes::instance()->view.width]);
            for (int i = 0; i < 3; i++)
              colorBuf[r.id].rgba[i] += r.color.rgba[i];
            colorBuf[r.id].rgba[3] = 1.f;
            colorBuf[r.id].clamp();
          }
        }
        for (auto& q : local_queue) {
          boost::mutex::scoped_lock(queue_mutex[q.first]);
          // GVT_DEBUG(DBG_ALWAYS, "Add " << q.second.size() << " to queue "
          //                              << q.first << " width size "
          //                              << queue[q.first].size() << "[" << mpi.rank << "]");
          queue[q.first]
              .insert(queue[q.first].end(), q.second.begin(), q.second.end());
        }
      // }));
    }
    rays.clear();
    //for (auto& f : futures) f.wait();
    GVT_DEBUG(DBG_ALWAYS,"["<< mpi.rank << "] Shuffle exit");
  }

  virtual bool SendRays() { GVT_ASSERT_BACKTRACE(0, "Not supported"); }

  virtual void localComposite() {
    const size_t size = width*height;
    //const size_t size = gvt::render::Attributes::rta->view.width *
    //                   gvt::render::Attributes::rta->view.height;

    // for (size_t i = 0; i < size; i++) image.Add(i, colorBuf[i]);

    int nchunks = std::thread::hardware_concurrency() * 2;
    int chunk_size = size / nchunks;
    std::vector< std::pair<int, int> > chunks;
    std::vector< std::future<void> > futures;
    for (int ii = 0; ii < nchunks - 1; ii++) {
      chunks.push_back(
          std::make_pair(ii * chunk_size, ii * chunk_size + chunk_size));
    }
    int ii = nchunks - 1;
    chunks.push_back(std::make_pair(ii * chunk_size, size));

    for (auto limit : chunks) {
      GVT_DEBUG(DBG_ALWAYS, "Limits : " << limit.first << ", " << limit.second);
      futures.push_back(std::async(std::launch::deferred, [&]() {
        for (int i = limit.first; i < limit.second; i++)
          image.Add(i, colorBuf[i]);
      }));
    }

    for (std::future<void>& f : futures) {
      f.wait();
    }
  }

  virtual void gatherFramebuffers(int rays_traced) {

    size_t size = width * height;
    //size_t size = gvt::render::Attributes::rta->view.width *
    //              gvt::render::Attributes::rta->view.height;


    for(size_t i =0; i < size; i++) image.Add(i, colorBuf[i]);                  

    if (!mpi) return;

    unsigned char* rgb = image.GetBuffer();

    int rgb_buf_size = 3 * size;

    unsigned char* bufs =
        mpi.root() ? new unsigned char[mpi.world_size * rgb_buf_size] : NULL;

    // MPI_Barrier(MPI_COMM_WORLD);
    MPI_Gather(rgb, rgb_buf_size, MPI_UNSIGNED_CHAR, bufs, rgb_buf_size,
               MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);
    if (mpi.root()) {
      int nchunks = std::thread::hardware_concurrency() * 2;
      int chunk_size = size / nchunks;
      std::vector< std::pair<int, int> > chunks(nchunks);
      std::vector< std::future<void> > futures;
      for (int ii = 0; ii < nchunks - 1; ii++) {
        chunks.push_back(
            std::make_pair(ii * chunk_size, ii * chunk_size + chunk_size));
      }
      int ii = nchunks - 1;
      chunks.push_back(std::make_pair(ii * chunk_size, size));

      for (auto& limit : chunks) {
        futures.push_back(std::async(std::launch::async, [&]() {
      //std::pair<int,int> limit = std::make_pair(0,size);
          for (int i = 1; i < mpi.world_size; ++i) {
            for (int j = limit.first * 3; j < limit.second * 3; j += 3) {
              int p = i * rgb_buf_size + j;
              // assumes black background, so adding is fine (r==g==b== 0)
              rgb[j + 0] += bufs[p + 0];
              rgb[j + 1] += bufs[p + 1];
              rgb[j + 2] += bufs[p + 2];
            }
          }
        }));
      }
      for (std::future<void>& f : futures) {
        f.wait();
      }
    }

    delete[] bufs;
  }
};


/// Generic Tracer interface for a base scheduling strategy with static inner
/// scheduling policy

/*! Tracer implementation generic interface
 *
 * \tparam DomainType Data domain type. Besides defining the domain behavior
 *defines the procedure to process the current queue of rays
 * \tparam MPIW MPI Communication World (Single node or Multiple Nodes)
 * \tparam BSCHEDUDER Base tracer scheduler (e.g. Image, Domain or Hybrid)
 *
 */

template <class BSCHEDULER>
class Tracer : public AbstractTrace {
 public:
  Tracer(gvt::render::actor::RayVector& rays,
         gvt::render::data::scene::Image& image)
      : AbstractTrace(rays, image) {}

  virtual ~Tracer() {}
};
/// Generic Tracer interface for a base scheduling strategy with mutable inner
/// scheduling policy

/*! Tracer implementation generic interface for scheduler with mutable inner
 *scheduling policy
 *
 * \tparam DomainType Data domain type. Besides defining the domain behavior
 *defines the procedure to process the current queue of rays
 * \tparam MPIW MPI Communication World (Single node or Multiple Nodes)
 * \tparam BSCHEDUDER Base tracer scheduler (e.g.Hybrid)
 * \tparam ISCHEDUDER Inner scheduler for base scheduler (Greedy, Spread, ...)
 *
 */
template <template <typename> class BSCHEDULER, class ISCHEDULER>
class Tracer< BSCHEDULER<ISCHEDULER> > : public AbstractTrace {
 public:
  Tracer(gvt::render::actor::RayVector& rays,
         gvt::render::data::scene::Image& image)
      : AbstractTrace(rays, image) {}

  virtual ~Tracer() {}
};
}
}
}

#endif /* GVT_RENDER_ALGORITHM_TRACER_BASE_H */
