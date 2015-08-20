// 
// Simple gravit application. 
// Load some geometry and render it. 
//
#include <gvt/render/RenderContext.h>
#include <gvt/render/Types.h>
#include <vector>
#include <algorithm>
#include <set>
#include <gvt/core/mpi/Wrapper.h>
#include <gvt/core/Math.h>
#include <gvt/render/data/Dataset.h>
#include <gvt/render/data/Domains.h>
#include <gvt/render/Schedulers.h>
#include <gvt/render/adapter/manta/Wrapper.h>
//#include <gvt/render/adapter/optix/Wrapper.h>
#include <gvt/render/algorithm/Tracers.h>
#include <gvt/render/data/scene/gvtCamera.h>
#include <gvt/render/data/scene/Image.h>
#include <gvt/render/data/Primitives.h>

#include <iostream>

using namespace std;
using namespace gvt::render;
using namespace gvt::core::math;
using namespace gvt::core::mpi;
using namespace gvt::render::data::scene;
using namespace gvt::render::schedule;
using namespace gvt::render::data::primitives;
using namespace gvt::render::adapter::manta::data::domain;
//using namespace gvt::render::adapter::optix::data::domain;


int main(int argc, char** argv) {

//
//	our friend the cone.
//
	Point4f points[7];
	points[0] = Point4f(0.5,0.0,0.0,1.0);
	points[1] = Point4f(-0.5,0.5,0.0,1.0);
	points[2] = Point4f(-0.5,0.25,0.433013,1.0);
	points[3] = Point4f(-0.5,-0.25,0.43013,1.0);
	points[4] = Point4f(-0.5,-0.5,0.0,1.0);
	points[5] = Point4f(-0.5,-0.25,-0.433013,1.0);
	points[6] = Point4f(-0.5,0.25,-0.433013,1.0);
//
//	build a mesh object from cone geometry.
//
	Mesh* objMesh = new Mesh(new Lambert(Vector4f(0.5,0.5,0.5,1.0)));
	objMesh->addVertex(points[0]);
	objMesh->addVertex(points[1]);
	objMesh->addVertex(points[2]);
	objMesh->addVertex(points[3]);
	objMesh->addVertex(points[4]);
	objMesh->addVertex(points[5]);
	objMesh->addVertex(points[6]);
	
	objMesh->addFace(1,2,3);
	objMesh->addFace(1,3,4);
	objMesh->addFace(1,4,5);
	objMesh->addFace(1,5,6);
	objMesh->addFace(1,6,7);
	objMesh->addFace(1,7,2);

	objMesh->generateNormals();

	MPI_Init(&argc, &argv);
//
//	scene contains the geometry domain and the other elements like
//	camera, lights, etc.
//
	gvt::render::data::Dataset scene;
//
//	create a geometry domain and place the mesh inside
//
	gvt::render::data::domain::GeometryDomain* domain = new gvt::render::data::domain::GeometryDomain(objMesh);
	scene.domainSet.push_back(domain );
//
//	Add a point light at 1,1,1 and add it to the domain.
//
	Vector4f   pos(1.0,1.0,1.0,0.0);
	Vector4f color(1.0,1.0,1.0,0.0);
	vector<gvt::render::data::scene::Light*> lightset;
	lightset.push_back(new gvt::render::data::scene::PointLight(pos,color));
	domain->setLights(lightset);
//
//	need a camera... using gvtCamera instead of default camera.... because I know how it works.  
//
	gvtPerspectiveCamera mycamera;
	Point4f cameraposition(1.0,1.0,1.0,1.0);
	Point4f focus(0.0,0.0,0.0,1.0);
	//float fov = 45.0 * M_PI/180.0; camera now expects angle in degrees
	float fov = 45.0 ;
	Vector4f up(0.0,1.0,0.0,0.0);
	mycamera.lookAt(cameraposition,focus,up);
	mycamera.setFOV(fov);
	mycamera.setFilmsize(512,512);
//
//	Create an object to hold the image and a pointer to the raw image data.
//
	Image myimage(mycamera.getFilmSizeWidth(),mycamera.getFilmSizeHeight(),"cone"); 
	unsigned char *imagebuffer = myimage.GetBuffer();
//
//	Attributes class contains information used by tracer to generate image.
//	This will be replaced by the Context in the future. 
//
    // RenderContext::CreateContext() ;
    gvt::render::RenderContext *cntxt = gvt::render::RenderContext::instance();

    if(cntxt == NULL) {
    	std::cout << "Something went wrong" << std::endl;
    	exit(0);
    }

	gvt::core::DBNodeH root = cntxt->getRootNode();
	gvt::core::Variant V;
	gvt::core::DBNodeH camnode,filmnode,datanode;

// camera
	camnode = cntxt->createNodeFromType("Camera","conecam",root.UUID());
	camnode["eyePoint"] = mycamera.getEyePoint();
	camnode["focus"] = mycamera.getFocalPoint();
	camnode["upVector"] = mycamera.getUpVector();
// film
	filmnode = cntxt->createNodeFromType("Film","conefilm",root.UUID());
	filmnode["width"] = mycamera.getFilmSizeWidth();
	filmnode["height"] = mycamera.getFilmSizeHeight();
// dataset
  datanode = cntxt->createNodeFromType("Dataset","coneset",root.UUID());
  datanode["schedule"] = gvt::render::scheduler::Image;
  datanode["Dataset_Pointer"] = new gvt::render::data::Dataset();
	datanode["render_type"] = gvt::render::adapter::Manta;


    if(gvt::core::variant_toInteger(datanode["render_type"].value()) == gvt::render::adapter::Manta) {
	    gvt::core::variant_toDatasetPointer(root["Dataset"]["Dataset_Pointer"].value())->addDomain(new MantaDomain(domain));
	}
  datanode["topology"] = Vector3f(1.,1.,1.);
	datanode["accel_type"] = gvt::render::accelerator::BVH;
	datanode["Mesh_Pointer"] = domain->getMesh();

	int width = gvt::core::variant_toInteger(root["Film"]["width"].value());
	std::cout << "this should print the tree " << width << std::endl;
	cntxt->database()->printTree(root.UUID(),10,std::cout);
//
//	Render it....
//
	mycamera.AllocateCameraRays();
	mycamera.generateRays();
	int stype = gvt::core::variant_toInteger(root["Dataset"]["schedule"].value());
	if(stype ==gvt::render::scheduler::Image) {
		gvt::render::algorithm::Tracer<ImageScheduler>(mycamera.rays,myimage)();
	} else if(stype == gvt::render::scheduler::Domain) {
		gvt::render::algorithm::Tracer<DomainScheduler>(mycamera.rays,myimage)();
	}
	myimage.Write();
}
