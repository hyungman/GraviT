/*
 * ImageTracer.h
 *
 *  Created on: Nov 27, 2013
 *      Author: jbarbosa
 */

#ifndef GVT_RENDER_ALGORITHM_IMAGE_TRACER_H
#define GVT_RENDER_ALGORITHM_IMAGE_TRACER_H

#include <mpi.h>


#include <gvt/core/mpi/Wrapper.h>
#include <gvt/render/Schedulers.h>
#include <gvt/render/algorithm/TracerBase.h>

#include <boost/timer/timer.hpp>

namespace gvt {
    namespace render {
        namespace algorithm {
            /// Tracer Image (ImageSchedule) based decomposition implementation

            template<> class Tracer<gvt::render::schedule::ImageScheduler> : public AbstractTrace
            {
            public:

                size_t rays_start, rays_end ;

                Tracer(gvt::render::actor::RayVector& rays, gvt::render::data::scene::Image& image) 
                : AbstractTrace(rays, image) 
                {
                    int ray_portion = rays.size() / mpi.world_size;
                    rays_start = mpi.rank * ray_portion;
                    rays_end = (mpi.rank + 1) == mpi.world_size ? rays.size() : (mpi.rank + 1) * ray_portion; // tack on any odd rays to last proc
                }

                virtual void FilterRaysLocally() {
                    if(mpi) {
                        gvt::render::actor::RayVector lrays;
                    		GVT_DEBUG(DBG_ALWAYS," Local Filter rays : " << rays.size() << std::endl);
                    		GVT_DEBUG(DBG_ALWAYS," Local Filter start end : " << rays_start << " " << rays_end << std::endl);
                        lrays.assign(rays.begin() + rays_start,
                                     rays.begin() + rays_end);
                        rays.clear();
                    		GVT_DEBUG(DBG_ALWAYS," Local Filter lrays: " << lrays.size() << std::endl);
                        shuffleRays(lrays);        
                    } else {
                        shuffleRays(rays);    
                    }
                }

                virtual void operator()() 
                {
                    GVT_DEBUG(DBG_ALWAYS,"Using Image schedule");
                    boost::timer::auto_cpu_timer t;

                    long ray_counter = 0, domain_counter = 0;


                    FilterRaysLocally();

                    // buffer for color accumulation
                    gvt::render::actor::RayVector moved_rays;
                    int domTarget = -1, domTargetCount = 0;
                    // process domains until all rays are terminated
                    do 
                    {
                        // process domain with most rays queued
                        domTarget = -1;
                        domTargetCount = 0;

                        GVT_DEBUG(DBG_ALWAYS, "Selecting new domain");
                        for (std::map<int, gvt::render::actor::RayVector>::iterator q = this->queue.begin(); q != this->queue.end(); ++q) 
                        {
                            if (q->second.size() > domTargetCount) 
                            {
                                domTargetCount = q->second.size();
                                domTarget = q->first;
                            }
                        }
                        //GVT_DEBUG(DBG_ALWAYS, "Selecting new domain");

                        if (domTarget >= 0) 
                        {

                            GVT_DEBUG(DBG_ALWAYS, "Getting domain " << domTarget << std::endl);
                            gvt::render::data::domain::AbstractDomain* dom = data->getDomain(domTarget);
                            dom->load();
                            GVT_DEBUG(DBG_ALWAYS, "dom: " << domTarget << std::endl);

                            // track domain loads
                            ++domain_counter;

                            GVT_DEBUG(DBG_ALWAYS, "Calling process queue");
                            {
                                moved_rays.reserve(this->queue[domTarget].size()*10);
                                boost::timer::auto_cpu_timer t("Tracing domain rays %t\n");
                                dom->trace(this->queue[domTarget], moved_rays);
                            }
                            GVT_DEBUG(DBG_ALWAYS, "Marching rays");
                            shuffleRays(moved_rays,dom);
                            moved_rays.clear();
                        }
                    } while (domTarget != -1);
                    GVT_DEBUG(DBG_ALWAYS, "Gathering buffers");
                    this->gatherFramebuffers(this->rays.size());
                }
            };
        }
    }
}
#endif /* GVT_RENDER_ALGORITHM_IMAGE_TRACER_H */
