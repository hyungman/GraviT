
#include <gvt/render/composite/composite.h>
#include <iostream>
#include <mpi.h>

#include <IceT.h>
#include <IceTDevCommunication.h>
#include <IceTDevContext.h>
#include <IceTDevDiagnostics.h>
#include <IceTDevImage.h>
#include <IceTDevPorting.h>
#include <IceTDevState.h>
#include <IceTMPI.h>

#include <glm/glm.hpp>
namespace gvt {
namespace render {
namespace composite {

static glm::vec4 *local_buffer;
const IceTDouble identity[] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };
const IceTFloat black[] = { 0.0, 0.0, 0.0, 1.0 };

static void draw(const IceTDouble *projection_matrix, const IceTDouble *modelview_matrix,
                 const IceTFloat *background_color, const IceTInt *readback_viewport, IceTImage result) {
  IceTFloat *color_buffer;
  IceTSizeType num_pixels;
  IceTSizeType i;

  /* Suppress compiler warnings. */
  (void)projection_matrix;
  (void)modelview_matrix;
  (void)background_color;
  (void)readback_viewport;

  num_pixels = icetImageGetNumPixels(result);

  color_buffer = icetImageGetColorf(result);

  for (i = 0; i < num_pixels; i++) {
    color_buffer[i * 4 + 0] = local_buffer[i][0];
    color_buffer[i * 4 + 1] = local_buffer[i][1];
    color_buffer[i * 4 + 2] = local_buffer[i][2];
    color_buffer[i * 4 + 3] = local_buffer[i][3];
  }
}

bool composite::initIceT() {
  if (MPI::COMM_WORLD.Get_size() < 2) return false;

  IceTInt *process_ranks;
  IceTInt proc;
  IceTCommunicator comm = icetCreateMPICommunicator(MPI_COMM_WORLD);
  icetCreateContext(comm);
  icetDestroyMPICommunicator(comm);

  icetGetIntegerv(ICET_NUM_PROCESSES, &num_proc);
  icetCompositeMode(ICET_COMPOSITE_MODE_BLEND);
  icetSetColorFormat(ICET_IMAGE_COLOR_RGBA_FLOAT);
  icetSetDepthFormat(ICET_IMAGE_DEPTH_NONE);
  icetDrawCallback(draw);
  icetStrategy(ICET_STRATEGY_DIRECT);
  icetSingleImageStrategy(ICET_SINGLE_IMAGE_STRATEGY_AUTOMATIC);

  // IceTSizeType num_pixels = icetImageGetNumPixels(result);
  // icetDisable(ICET_CORRECT_COLORED_BACKGROUND);
  // icetDisable(ICET_ORDERED_COMPOSITE);

  return true;
}

glm::vec4 *composite::execute(glm::vec4 *buffer_in, const size_t width, const size_t height) {
  IceTFloat *color_buffer;
  IceTSizeType num_pixels;
  IceTSizeType i;

  local_buffer = buffer_in;
  icetResetTiles();
  icetAddTile(0, 0, width, height, 0);

  IceTImage result = icetDrawFrame(identity, identity, black);

  num_pixels = icetImageGetNumPixels(result);
  color_buffer = icetImageGetColorf(result);

  glm::vec4 *final_image = new glm::vec4[width * height];
  for (i = 0; i < num_pixels; i++) {
    final_image[i][0] = color_buffer[i * 4 + 0];
    final_image[i][1] = color_buffer[i * 4 + 1];
    final_image[i][2] = color_buffer[i * 4 + 2];
    final_image[i][3] = color_buffer[i * 4 + 3];
  }

  return final_image;
}
}
}
}
