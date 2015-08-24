//
//  GVTTrace.C
//

#include "mpe.h"
#include "ConfigFileLoader.h"
#include "MantaRayTracer.h"
#include "OptixRayTracer.h"
#include "EmbreeRayTracer.h"

#include <gvt/core/Math.h>
#ifdef GVT_RENDER_ADAPTER_MANTA
#include <gvt/render/adapter/manta/Wrapper.h>
#endif
#ifdef GVT_RENDER_ADAPTER_OPTIX
#include <gvt/render/adapter/optix/Wrapper.h>
#endif
#ifdef GVT_RENDER_ADAPTER_EMBREE
#include <gvt/render/adapter/embree/Wrapper.h>
#endif
#include <gvt/render/data/Primitives.h>

#include <mpi.h>

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;
using namespace gvtapps::render;

int main(int argc, char** argv) {

	int event1a, event1b, event2a, event2b;
	int event1, event2;
	int rank = -1;
  MPI_Init(&argc, &argv);
	MPE_Init_log();
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Barrier(MPI_COMM_WORLD);

  string filename, imagename;

  if (argc > 1)
    filename = argv[1];
  else
    filename = "./gvttrace.conf";

  if (argc > 2)
    imagename = argv[2];
  else
    imagename = "GVTTrace";

	MPE_Log_get_state_eventIDs(&event1a, &event1b);
	MPE_Log_get_state_eventIDs(&event2a, &event2b);
  MPE_Log_get_solo_eventID( &event1);
  MPE_Log_get_solo_eventID( &event2);
	if(rank==0) {
		MPE_Describe_state( event1a, event1b, "read input","red");
		MPE_Describe_state( event2a, event2b, "Trace","green");
	}
	MPE_Log_event(event1,0,NULL);
	MPE_Log_event(event1a,0,NULL);
  gvtapps::render::ConfigFileLoader cl(filename);
	MPE_Log_event(event1b,0,NULL);

  bool domain_choosen = false;
#ifdef GVT_RENDER_ADAPTER_MANTA
  GVT_DEBUG(DBG_ALWAYS,"Rendering with Manta");
  if (cl.domain_type == 0) {
    domain_choosen = true;
    MantaRayTracer rt(cl);
    MPI_Barrier(MPI_COMM_WORLD);
		MPE_Log_event(event2,0,NULL);
		MPE_Log_event(event2a,0,NULL);
    rt.RenderImage(imagename);
		MPE_Log_event(event2b,0,NULL);
  }
#endif
#ifdef GVT_RENDER_ADAPTER_OPTIX
  GVT_DEBUG(DBG_ALWAYS,"Rendering with OptiX");
  if (cl.domain_type == 1) {
    domain_choosen = true;
    OptixRayTracer rt(cl);
    MPI_Barrier(MPI_COMM_WORLD);
    rt.RenderImage(imagename);
  }
#endif
#ifdef GVT_RENDER_ADAPTER_EMBREE
  GVT_DEBUG(DBG_ALWAYS,"Rendering with Embree");
  if (cl.domain_type == 2) {
    domain_choosen = true;
    EmbreeRayTracer rt(cl);
    MPI_Barrier(MPI_COMM_WORLD);
    rt.RenderImage(imagename);
  }
#endif

  GVT_ASSERT(domain_choosen,"The requested domain type is not available, please recompile");
	MPE_Log_sync_clocks();
	MPE_Finish_log("gvtTracerlog");
  if (MPI::COMM_WORLD.Get_size() > 1) MPI_Finalize();

  return 0;
}
