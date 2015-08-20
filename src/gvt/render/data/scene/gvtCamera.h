#ifndef GVT_RENDER_DATA_SCENE_GVTCAMERA_H
#define GVT_RENDER_DATA_SCENE_GVTCAMERA_H

// this file contains definitions for cameras used in GraviT. 
// 
// Dave Semeraro - May 2015
//
#include <gvt/core/Math.h>
#include <gvt/render/data/Primitives.h>
#include <stdlib.h>

namespace gvt {
	namespace render {
		namespace data {
			namespace scene {
			
			/// gvtCameraBase - A base class for GraviT cameras.
			/** The base class contains all the methods and variables common
			 *  to all cameras in GraviT. The camera class maintains all the camera state. It
			 *  also contains the vector of primary rays. An affine transformation matrix is
			 *  maintained that transforms from camera to world and from world to camera coordinate
			 *  systems. Left handed coordinate systems are assumed for both. */

			class gvtCameraBase
			{
			public:
				/** Default constructor sets eye_point and focal_point to ( 0, 0, 0, 1), 
 				 *  and view direction too (0, 0, -1, 0) (looking down z axis). The up
 				 *  vector is set to (0, 1, 0, 0) This is the OpenGL camera default. 
 				 */ 
				gvtCameraBase();
				/** Copy constructor */
				gvtCameraBase(const gvtCameraBase& cam);
				/** Destructor */
				~gvtCameraBase();
				/** Set film size film_size[0] is width */
				void setFilmsize(const int film_size[] );
				/** Set Film size from two ints */
				void setFilmsize(int width, int height);
				/** get the film width */
				int getFilmSizeWidth();
				/** get the film height */
				int getFilmSizeHeight();
				/** Set the eye point or position of the camera. This call causes recomputation of the 
				 *  transformation matrix, and all other camera parameters impacted by the change. */
				void setEye(const gvt::core::math::Vector4f &eye);
				/** Pass in the camera ray vector reference so the camera can populate it with rays.
 				 *  This method is not really necessary and is only here for backward compatibility. */
				void SetCamera(gvt::render::actor::RayVector &rays, float rate);
				/** Return a random floating point value between 0.0 and 1.0 */
				float frand();
				/** Fill up the ray vector with correct rays. Base class just initializes the vector.
                                 *  Derived classes insert the rays themselves. */
				//virtual gvt::render::actor::RayVector AllocateCameraRays();
				virtual void AllocateCameraRays();
				/** given a new eye point, focal point, and up vector modify all the other dependant vars. 
 				 *  in particular rebuild the transformation matrix. The new camera position is passed in as
 				 *  eye. The new focal point is passed in as focus. And the camera up vector is passed in as up. The
 				 *  camera coordinate system with unit vectors, u, v, and w is constructed. From this the camera
 				 *  to world transformation and its inverse are constructed. */
				void lookAt(gvt::core::math::Point4f eye, gvt::core::math::Point4f focus, gvt::core::math::Vector4f up);

				/** Bunch-o-rays */
				gvt::render::actor::RayVector rays;
				gvt::core::math::Point4f getEyePoint() {return eye_point;};
				gvt::core::math::Point4f getFocalPoint() {return focal_point;};
				gvt::core::math::Vector4f getUpVector() {return up_vector;};
			protected:
				gvt::core::math::AffineTransformMatrix<float> cam2wrld; //!< transform from camera to world coords
				gvt::core::math::AffineTransformMatrix<float> wrld2cam; //!< transform from world to camera coords
				gvt::core::math::Point4f eye_point;      //!< camera location in world coordinates
				gvt::core::math::Point4f focal_point;    //!< camera focal point in world coordinates
				gvt::core::math::Vector4f up_vector;      //!< vector pointing "up". 
				gvt::core::math::Vector4f view_direction; //!< direction camera is looking. generally focal - eye pt. 
				int filmsize[2];                          //!< image size dimensions in pixels. filmsize[0] = width. 
				int depth;				  //!< legacy variable from previous cameras. Initializes ray depth
				gvt::core::math::Vector4f u,v,w;	  //!< unit basis vectors for camera space in world coords. 
				float INVRAND_MAX;
			//
				void buildTransform();			  //!< Build the transformation matrix and inverse
			}; 

			/// gvtPerspectiveCamera - a camera that produces a perspective view
			/** This camera produces a perspective view of the world. The field of view is the 
			 *  angle subtended by the film plane width from the eye point. This class has methods
			 *  to allocate rays and to initialize or generate the primary ray set based on the 
			 *  camera state. 
			 */
			class gvtPerspectiveCamera: public gvtCameraBase
			{
			public:
				/** Perspective camera default constructor */
				gvtPerspectiveCamera();
				/** Perspective camera copy constructor */
				gvtPerspectiveCamera(const gvtPerspectiveCamera& cam);
				/** Perspective camera destructor */
				~gvtPerspectiveCamera();
				/** Set the field of view angle in degrees*/
				void setFOV(const float fov);
				/** Fill the ray data structure */
				virtual void generateRays();
			protected:
				float field_of_view;			  //!< Angle subtended by the film plane height from eye_point 
				double m_pi ;
			};

			} // scene
		} // data
	} // render
} // gvt


#endif //GVT_RENDER_DATA_SCENE_GVTCAMERA_H
