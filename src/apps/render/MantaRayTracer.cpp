//
//  RayTracer.C
//

#include "MantaRayTracer.h"

#include <gvt/core/mpi/Wrapper.h>
#include <gvt/render/adapter/manta/Wrapper.h>
#include <gvt/render/algorithm/Tracers.h>
#include <gvt/render/data/scene/Camera.h>
#include <gvt/render/data/scene/Image.h>
#include <gvt/render/Schedulers.h>

// Manta includes
#include <Interface/LightSet.h>
#include <Model/Lights/PointLight.h>
#include <Model/Materials/Phong.h>
#include <Model/Readers/PlyReader.h>
// end Manta includes

#include <boost/foreach.hpp>

#ifdef PARALLEL
#include <mpi.h>
#endif

using namespace gvtapps::render;
using namespace gvt::core::mpi;
using namespace gvt::render::adapter::manta::data::domain;
using namespace gvt::render::data::domain;
using namespace gvt::render::data::scene;
using namespace gvt::render::schedule;

MantaRayTracer::MantaRayTracer(gvt::render::data::Dataset* scene) : scene(scene) 
{
    scene->camera.SetCamera(rays,1.0);
    
    gvt::render::Attributes& rta = *(gvt::render::Attributes::instance());
    
    rta.dataset = new gvt::render::data::Dataset();
    
    
    BOOST_FOREACH(AbstractDomain* dom, scene->domainSet) 
    {
        GeometryDomain* d = (GeometryDomain*)dom;
        d->setLights(scene->lightSet);
        rta.dataset->addDomain(new MantaDomain(d));
    }
    
    
    rta.view.width = scene->camera.getFilmSizeWidth();
    rta.view.height = scene->camera.getFilmSizeHeight();
    rta.view.camera = scene->camera.getEye();
    rta.view.focus = scene->camera.getLook();
    rta.view.up = scene->camera.up;
    
    rta.sample_rate = 1.0f;
    rta.sample_ratio = 1.0f;
    
    rta.do_lighting = true;
    rta.schedule = gvt::render::Attributes::Image;
    rta.render_type = gvt::render::Attributes::Manta;
    
    rta.datafile = "";
}

void MantaRayTracer::RenderImage(std::string imagename = "mpitrace") 
{
    
    boost::timer::auto_cpu_timer t("Total render time: %t\n");
    
    Image image(scene->camera.getFilmSizeWidth(),scene->camera.getFilmSizeHeight(), imagename);
    rays = scene->camera.MakeCameraRays();
    gvt::render::algorithm::Tracer<MantaDomain, MPICOMM, ImageScheduler>(rays, image)();  
    image.Write();
    
    //Example code. Too complex.
    
    
//    
//    Image image(GVT::Env::RayTracerAttributes::rta->view.width, GVT::Env::RayTracerAttributes::rta->view.height, imagename);
//   
//    GVT::Env::Camera<C_PERSPECTIVE> cam(rays, GVT::Env::RayTracerAttributes::rta->view, GVT::Env::RayTracerAttributes::rta->sample_rate);
//    cam.MakeCameraRays();
//
//    int render_type = GVT::Env::RayTracerAttributes::rta->render_type;
//    
//
//    switch (GVT::Env::RayTracerAttributes::rta->schedule) {
//        case GVT::Env::RayTracerAttributes::Image:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, ImageSchedule>(rays, image)();
//            else
//                GVT::Trace::Tracer<GVT::Domain::VolumeDomain, MPICOMM, ImageSchedule>(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::Domain:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, DomainSchedule>(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, DomainSchedule>(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::Greedy:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, HybridSchedule<GreedySchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<GreedySchedule> >(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::Spread:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, HybridSchedule<SpreadSchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<SpreadSchedule> >(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::RayWeightedSpread:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, HybridSchedule<RayWeightedSpreadSchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<RayWeightedSpreadSchedule> >(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::AdaptiveSend:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, HybridSchedule<AdaptiveSendSchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<AdaptiveSendSchedule> >(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::LoadOnce:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM,  HybridSchedule<LoadOnceSchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<LoadOnceSchedule> >(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::LoadAnyOnce:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, HybridSchedule<LoadAnyOnceSchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<LoadAnyOnceSchedule> >(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::LoadAnother:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, HybridSchedule<LoadAnotherSchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<LoadAnotherSchedule> >(rays, image)();
//            break;
//        case GVT::Env::RayTracerAttributes::LoadMany:
//            if (render_type == GVT::Env::RayTracerAttributes::Manta)
//                GVT::Trace::Tracer<GVT::Domain::MantaDomain, MPICOMM, HybridSchedule<LoadManySchedule> >(rays, image)();
//            else
//                GVT::Trace::Tracer< GVT::Domain::VolumeDomain, MPICOMM, HybridSchedule<LoadManySchedule> >(rays, image)();
//            break;
//        default:
//            cerr << "ERROR: unknown schedule '" << GVT::Env::RayTracerAttributes::rta->schedule << "'" << endl;
//            return;
//    }
//
//
//    int rank;
//    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//    if (rank == 0) {
//        image.Write();
//    }

};

#if !defined(M_PI)
#define M_PI 3.14159265358979323846
#endif



