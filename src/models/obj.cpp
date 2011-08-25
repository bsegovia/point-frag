#include "obj.hpp"
#include "sys/platform.hpp"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cassert>
//#include <unordered_map>
#include <map>
#include <vector>
#include <algorithm>

#define WHITESPACE " \t\n\r"

using namespace pf;

enum {
  OBJ_FILENAME_LENGTH = 500,
  MATERIAL_NAME_SIZE = 255,
  OBJ_LINE_SIZE = 500,
  MAX_VERTEX_COUNT = 4
};

struct list {
  int item_count;
  int current_max_size;
  char growable;
  void **items;
  char **names;	
};

struct obj_face {
  int vertex_index[MAX_VERTEX_COUNT];
  int normal_index[MAX_VERTEX_COUNT];
  int texture_index[MAX_VERTEX_COUNT];
  int vertex_count;
  int material_index;
};

struct obj_sphere {
  int pos_index;
  int up_normal_index;
  int equator_normal_index;
  int texture_index[MAX_VERTEX_COUNT];
  int material_index;
};

struct obj_plane {
  int pos_index;
  int normal_index;
  int rotation_normal_index;
  int texture_index[MAX_VERTEX_COUNT];
  int material_index;
};

struct obj_vector { double e[3]; };

struct obj_material {
  char name[MATERIAL_NAME_SIZE];
  char map_Ka[OBJ_FILENAME_LENGTH];
  char map_Kd[OBJ_FILENAME_LENGTH];
  char map_D[OBJ_FILENAME_LENGTH];
  char map_Bump[OBJ_FILENAME_LENGTH];
  double amb[3];
  double diff[3];
  double spec[3];
  double km;
  double reflect;
  double refract;
  double trans;
  double shiny;
  double glossy;
  double refract_index;
};

struct obj_camera {
  int camera_pos_index;
  int camera_look_point_index;
  int camera_up_norm_index;
};

struct obj_light_point {
  int pos_index;
  int material_index;
};

struct obj_light_disc {
  int pos_index;
  int normal_index;
  int material_index;
};

struct obj_light_quad {
  int vertex_index[MAX_VERTEX_COUNT];
  int material_index;
};

struct obj_growable_scene_data {
  char scene_filename[OBJ_FILENAME_LENGTH];
  char material_filename[OBJ_FILENAME_LENGTH];

  list vertex_list;
  list vertex_normal_list;
  list vertex_texture_list;

  list face_list;
  list sphere_list;
  list plane_list;

  list light_point_list;
  list light_quad_list;
  list light_disc_list;

  list material_list;

  obj_camera *camera;
};

struct obj_scene_data {
  obj_vector **vertex_list;
  obj_vector **vertex_normal_list;
  obj_vector **vertex_texture_list;

  obj_face **face_list;
  obj_sphere **sphere_list;
  obj_plane **plane_list;

  obj_light_point **light_point_list;
  obj_light_quad **light_quad_list;
  obj_light_disc **light_disc_list;

  obj_material **material_list;

  int vertex_count;
  int vertex_normal_count;
  int vertex_texture_count;

  int face_count;
  int sphere_count;
  int plane_count;

  int light_point_count;
  int light_quad_count;
  int light_disc_count;

  int material_count;

  obj_camera *camera;
};

class objLoader
{
public:

  objLoader();
  ~objLoader();
  int load(const char *filename);

  obj_vector **vertexList;
  obj_vector **normalList;
  obj_vector **textureList;

  obj_face **faceList;
  obj_sphere **sphereList;
  obj_plane **planeList;

  obj_light_point **lightPointList;
  obj_light_quad **lightQuadList;
  obj_light_disc **lightDiscList;

  obj_material **materialList;

  int vertexCount;
  int normalCount;
  int textureCount;

  int faceCount;
  int sphereCount;
  int planeCount;

  int lightPointCount;
  int lightQuadCount;
  int lightDiscCount;

  int materialCount;

  obj_camera *camera;
  obj_scene_data data;
};

static char strequal(const char *s1, const char *s2)
{
  if(strcmp(s1, s2) == 0)
    return 1;
  return 0;
}

static char contains(const char *haystack, const char *needle)
{
  if(strstr(haystack, needle) == NULL)
    return 0;
  return 1;
}

static void list_make(list *listo, int size, char growable);
static int list_add_item(list *listo, void *item, char *name);
static int list_find(list *listo, char *name_to_find);
static void list_delete_index(list *listo, int indx);
static void list_delete_all(list *listo);
static void list_free(list *listo);

static char list_is_full(list *listo)
{
  return(listo->item_count == listo->current_max_size);
}

static void list_make(list *listo, int start_size, char growable)
{
  listo->names = (char**) malloc(sizeof(char*) * start_size);
  listo->items = (void**) malloc(sizeof(void*) * start_size);
  listo->item_count = 0;
  listo->current_max_size = start_size;
  listo->growable = growable;
}

static void list_grow(list *old_listo)
{
  int i;
  list new_listo;

  list_make(&new_listo, old_listo->current_max_size*2, old_listo->growable++);

  for(i=0; i<old_listo->current_max_size; i++)
    list_add_item(&new_listo, old_listo->items[i] , old_listo->names[i]);

  list_free(old_listo);

  //copy new structure to old list
  old_listo->names = new_listo.names;
  old_listo->items = new_listo.items;
  old_listo->item_count = new_listo.item_count;
  old_listo->current_max_size = new_listo.current_max_size;
  old_listo->growable = new_listo.growable;
}

static int list_add_item(list *listo, void *item, char *name)
{
  int name_length;
  char *new_name;

  if( list_is_full(listo) )
  {
    if( listo->growable )
      list_grow(listo);
    else
      return -1;
  }

  listo->names[listo->item_count] = NULL;
  if(name != NULL)
  {
    name_length = strlen(name);
    new_name = (char*) malloc(sizeof(char) * name_length + 1);
    strncpy(new_name, name, name_length);
    listo->names[listo->item_count] = new_name;
  }

  listo->items[listo->item_count] = item;
  listo->item_count++;

  return listo->item_count-1;
}

static int list_find(list *listo, char *name_to_find)
{
  for (int i=0; i < listo->item_count; i++)
    if (strncmp(listo->names[i], name_to_find, strlen(name_to_find)) == 0)
      return i;
  return -1;
}

static void list_delete_index(list *listo, int indx)
{
  int j;

  //remove item
  if(listo->names[indx] != NULL)
    free(listo->names[indx]);

  //restructure
  for(j=indx; j < listo->item_count-1; j++) {
    listo->names[j] = listo->names[j+1];
    listo->items[j] = listo->items[j+1];
  }

  listo->item_count--;

  return;
}

static void list_delete_all(list *listo)
{
  int i;

  for(i=listo->item_count-1; i>=0; i--)
    list_delete_index(listo, i);
}

static void list_free(list *listo)
{
  list_delete_all(listo);
  free(listo->names);
  free(listo->items);
}

static void obj_free_half_list(list *listo)
{
  list_delete_all(listo);
  free(listo->names);
}

static int obj_convert_to_list_index(int current_max, int index)
{
  if(index == 0)  //no index
    return -1;

  if(index < 0)  //relative to current list position
    return current_max + index;

  return index - 1;  //normal counting index
}

static void obj_convert_to_list_index_v(int current_max, int *indices)
{
  for(int i=0; i<MAX_VERTEX_COUNT; i++)
    indices[i] = obj_convert_to_list_index(current_max, indices[i]);
}

static void obj_set_material_defaults(obj_material *mtl)
{
  mtl->amb[0] = 0.2;
  mtl->amb[1] = 0.2;
  mtl->amb[2] = 0.2;
  mtl->diff[0] = 0.8;
  mtl->diff[1] = 0.8;
  mtl->diff[2] = 0.8;
  mtl->spec[0] = 1.0;
  mtl->spec[1] = 1.0;
  mtl->spec[2] = 1.0;
  mtl->reflect = 0.0;
  mtl->trans = 1;
  mtl->glossy = 98;
  mtl->shiny = 0;
  mtl->refract_index = 1;
  mtl->map_Ka[0] = '\0';
}

static int obj_parse_vertex_index(int *vertex_index, int *texture_index, int *normal_index)
{
  char *temp_str;
  char *token;
  int vertex_count = 0;


  while( (token = strtok(NULL, WHITESPACE)) != NULL)
  {
    if(texture_index != NULL)
      texture_index[vertex_count] = 0;
    if(normal_index != NULL)
      normal_index[vertex_count] = 0;

    vertex_index[vertex_count] = atoi( token );

    if (contains(token, "//")) { //normal only
      temp_str = strchr(token, '/');
      temp_str++;
      normal_index[vertex_count] = atoi( ++temp_str );
    } else if(contains(token, "/")) {
      temp_str = strchr(token, '/');
      texture_index[vertex_count] = atoi( ++temp_str );
      if(contains(temp_str, "/")) {
        temp_str = strchr(temp_str, '/');
        normal_index[vertex_count] = atoi( ++temp_str );
      }
    }

    vertex_count++;
  }

  return vertex_count;
}

static obj_face* obj_parse_face(obj_growable_scene_data *scene)
{
  int vertex_count;
  obj_face *face = (obj_face*)malloc(sizeof(obj_face));

  vertex_count = obj_parse_vertex_index(face->vertex_index, face->texture_index, face->normal_index);
  obj_convert_to_list_index_v(scene->vertex_list.item_count, face->vertex_index);
  obj_convert_to_list_index_v(scene->vertex_texture_list.item_count, face->texture_index);
  obj_convert_to_list_index_v(scene->vertex_normal_list.item_count, face->normal_index);
  face->vertex_count = vertex_count;

  return face;
}

static obj_sphere* obj_parse_sphere(obj_growable_scene_data *scene)
{
  int temp_indices[MAX_VERTEX_COUNT];

  obj_sphere *obj = (obj_sphere*)malloc(sizeof(obj_sphere));
  obj_parse_vertex_index(temp_indices, obj->texture_index, NULL);
  obj_convert_to_list_index_v(scene->vertex_texture_list.item_count, obj->texture_index);
  obj->pos_index = obj_convert_to_list_index(scene->vertex_list.item_count, temp_indices[0]);
  obj->up_normal_index = obj_convert_to_list_index(scene->vertex_normal_list.item_count, temp_indices[1]);
  obj->equator_normal_index = obj_convert_to_list_index(scene->vertex_normal_list.item_count, temp_indices[2]);

  return obj;
}

static obj_plane* obj_parse_plane(obj_growable_scene_data *scene)
{
  int temp_indices[MAX_VERTEX_COUNT];

  obj_plane *obj = (obj_plane*)malloc(sizeof(obj_plane));
  obj_parse_vertex_index(temp_indices, obj->texture_index, NULL);
  obj_convert_to_list_index_v(scene->vertex_texture_list.item_count, obj->texture_index);
  obj->pos_index = obj_convert_to_list_index(scene->vertex_list.item_count, temp_indices[0]);
  obj->normal_index = obj_convert_to_list_index(scene->vertex_normal_list.item_count, temp_indices[1]);
  obj->rotation_normal_index = obj_convert_to_list_index(scene->vertex_normal_list.item_count, temp_indices[2]);

  return obj;
}

static obj_light_point* obj_parse_light_point(obj_growable_scene_data *scene)
{
  obj_light_point *o= (obj_light_point*)malloc(sizeof(obj_light_point));
  o->pos_index = obj_convert_to_list_index(scene->vertex_list.item_count, atoi( strtok(NULL, WHITESPACE)) );
  return o;
}

static obj_light_quad* obj_parse_light_quad(obj_growable_scene_data *scene)
{
  obj_light_quad *o = (obj_light_quad*)malloc(sizeof(obj_light_quad));
  obj_parse_vertex_index(o->vertex_index, NULL, NULL);
  obj_convert_to_list_index_v(scene->vertex_list.item_count, o->vertex_index);

  return o;
}

static obj_light_disc* obj_parse_light_disc(obj_growable_scene_data *scene)
{
  int temp_indices[MAX_VERTEX_COUNT];

  obj_light_disc *obj = (obj_light_disc*)malloc(sizeof(obj_light_disc));
  obj_parse_vertex_index(temp_indices, NULL, NULL);
  obj->pos_index = obj_convert_to_list_index(scene->vertex_list.item_count, temp_indices[0]);
  obj->normal_index = obj_convert_to_list_index(scene->vertex_normal_list.item_count, temp_indices[1]);

  return obj;
}

static obj_vector* obj_parse_vector()
{
  obj_vector *v = (obj_vector*)malloc(sizeof(obj_vector));
  v->e[0] = v->e[1] = v->e[2] = 0.;
  for (int i = 0; i < 3; ++i) {
    const char * str = strtok(NULL, WHITESPACE);
    if (str == NULL)
      break;
    v->e[i] = atof(str);
  }
  return v;
}

static void obj_parse_camera(obj_growable_scene_data *scene, obj_camera *camera)
{
  int indices[3];
  obj_parse_vertex_index(indices, NULL, NULL);
  camera->camera_pos_index = obj_convert_to_list_index(scene->vertex_list.item_count, indices[0]);
  camera->camera_look_point_index = obj_convert_to_list_index(scene->vertex_list.item_count, indices[1]);
  camera->camera_up_norm_index = obj_convert_to_list_index(scene->vertex_normal_list.item_count, indices[2]);
}

static int obj_parse_mtl_file(char *filename, list *material_list)
{
  int line_number = 0;
  char *current_token;
  char current_line[OBJ_LINE_SIZE];
  char material_open = 0;
  obj_material *current_mtl = NULL;
  FILE *mtl_file_stream;

  // open scene
  mtl_file_stream = fopen( filename, "r");
  if(mtl_file_stream == 0)
  {
    fprintf(stderr, "Error reading file: %s\n", filename);
    return 0;
  }

  list_make(material_list, 10, 1);

  while( fgets(current_line, OBJ_LINE_SIZE, mtl_file_stream) )
  {
    current_token = strtok( current_line, " \t\n\r");
    line_number++;

    //skip comments
    if( current_token == NULL || strequal(current_token, "//") || strequal(current_token, "#"))
      continue;
    //start material
    else if( strequal(current_token, "newmtl")) {
      material_open = 1;
      current_mtl = (obj_material*) malloc(sizeof(obj_material));
      obj_set_material_defaults(current_mtl);

      // get the name
      strncpy(current_mtl->name, strtok(NULL, " \t"), MATERIAL_NAME_SIZE);
      list_add_item(material_list, current_mtl, current_mtl->name);
    }
    //ambient
    else if( strequal(current_token, "Ka") && material_open) {
      current_mtl->amb[0] = atof( strtok(NULL, " \t"));
      current_mtl->amb[1] = atof( strtok(NULL, " \t"));
      current_mtl->amb[2] = atof( strtok(NULL, " \t"));
    }
    //diff
    else if( strequal(current_token, "Kd") && material_open) {
      current_mtl->diff[0] = atof( strtok(NULL, " \t"));
      current_mtl->diff[1] = atof( strtok(NULL, " \t"));
      current_mtl->diff[2] = atof( strtok(NULL, " \t"));
    }
    //specular
    else if( strequal(current_token, "Ks") && material_open) {
      current_mtl->spec[0] = atof( strtok(NULL, " \t"));
      current_mtl->spec[1] = atof( strtok(NULL, " \t"));
      current_mtl->spec[2] = atof( strtok(NULL, " \t"));
    }
    //shiny
    else if( strequal(current_token, "Ns") && material_open)
      current_mtl->shiny = atof( strtok(NULL, " \t"));
    // map_Km
    else if( strequal(current_token, "Km") && material_open)
      current_mtl->km = atof( strtok(NULL, " \t"));
    //transparent
    else if( strequal(current_token, "d") && material_open)
      current_mtl->trans = atof( strtok(NULL, " \t"));
    //reflection
    else if( strequal(current_token, "r") && material_open)
      current_mtl->reflect = atof( strtok(NULL, " \t"));
    //glossy
    else if( strequal(current_token, "sharpness") && material_open)
      current_mtl->glossy = atof( strtok(NULL, " \t"));
    //refract index
    else if( strequal(current_token, "Ni") && material_open)
      current_mtl->refract_index = atof( strtok(NULL, " \t"));
    // illumination type
    else if( strequal(current_token, "illum") && material_open)
    { }
    // map_Ka
    else if( strequal(current_token, "map_Ka") && material_open)
      strncpy(current_mtl->map_Ka, strtok(NULL, " \t"), OBJ_FILENAME_LENGTH);
    // map_Kd
    else if( strequal(current_token, "map_Kd") && material_open)
      strncpy(current_mtl->map_Kd, strtok(NULL, " \t"), OBJ_FILENAME_LENGTH);
    // map_D
    else if( strequal(current_token, "map_D") && material_open)
      strncpy(current_mtl->map_D, strtok(NULL, " \t"), OBJ_FILENAME_LENGTH);
    // map_Bump
    else if( strequal(current_token, "map_Bump") && material_open)
      strncpy(current_mtl->map_Bump, strtok(NULL, " \t"), OBJ_FILENAME_LENGTH);
    else
      fprintf(stderr, "Unknown command '%s' in material file %s at line %i:\n\t%s\n",
              current_token, filename, line_number, current_line);
  }

  fclose(mtl_file_stream);
  return 1;
}

static int obj_parse_obj_file(obj_growable_scene_data *growable_data, const char *filename)
{
  FILE* obj_file_stream;
  int current_material = -1; 
  char *current_token = NULL;
  char current_line[OBJ_LINE_SIZE];
  int line_number = 0;
  // open scene
  obj_file_stream = fopen( filename, "r");
  if(obj_file_stream == 0)
  {
    fprintf(stderr, "Error reading file: %s\n", filename);
    return 0;
  }

  //parser loop
  while( fgets(current_line, OBJ_LINE_SIZE, obj_file_stream) )
  {
    current_token = strtok( current_line, " \t\n\r");
    line_number++;

    //skip comments
    if( current_token == NULL || current_token[0] == '#')
      continue;

    //parse objects
    else if( strequal(current_token, "v") ) //process vertex
      list_add_item(&growable_data->vertex_list,  obj_parse_vector(), NULL);
    else if( strequal(current_token, "vn") ) //process vertex normal
      list_add_item(&growable_data->vertex_normal_list,  obj_parse_vector(), NULL);
    else if( strequal(current_token, "vt") ) //process vertex texture
      list_add_item(&growable_data->vertex_texture_list,  obj_parse_vector(), NULL);
    else if( strequal(current_token, "f") ) { //process face 
      obj_face *face = obj_parse_face(growable_data);
      face->material_index = current_material;
      list_add_item(&growable_data->face_list, face, NULL);
    }
    else if( strequal(current_token, "sp") ) { //process sphere
      obj_sphere *sphr = obj_parse_sphere(growable_data);
      sphr->material_index = current_material;
      list_add_item(&growable_data->sphere_list, sphr, NULL);
    }
    else if( strequal(current_token, "pl") ) { //process plane
      obj_plane *pl = obj_parse_plane(growable_data);
      pl->material_index = current_material;
      list_add_item(&growable_data->plane_list, pl, NULL);
    }
    else if( strequal(current_token, "p") ) //process point
    { }
    else if( strequal(current_token, "lp") ) { //light point source
      obj_light_point *o = obj_parse_light_point(growable_data);
      o->material_index = current_material;
      list_add_item(&growable_data->light_point_list, o, NULL);
    }
    else if( strequal(current_token, "ld") ) { //process light disc
      obj_light_disc *o = obj_parse_light_disc(growable_data);
      o->material_index = current_material;
      list_add_item(&growable_data->light_disc_list, o, NULL);
    }
    else if( strequal(current_token, "lq") ) { //process light quad
      obj_light_quad *o = obj_parse_light_quad(growable_data);
      o->material_index = current_material;
      list_add_item(&growable_data->light_quad_list, o, NULL);
    }
    else if( strequal(current_token, "c") ) { //camera
      growable_data->camera = (obj_camera*) malloc(sizeof(obj_camera));
      obj_parse_camera(growable_data, growable_data->camera);
    }
    else if( strequal(current_token, "usemtl") ) // usemtl
      current_material = list_find(&growable_data->material_list, strtok(NULL, WHITESPACE));
    else if( strequal(current_token, "mtllib") ) { // mtllib
      strncpy(growable_data->material_filename, strtok(NULL, WHITESPACE), OBJ_FILENAME_LENGTH);
      obj_parse_mtl_file(growable_data->material_filename, &growable_data->material_list);
      continue;
    }
    else if( strequal(current_token, "o") ) //object name
    { }
    else if( strequal(current_token, "s") ) //smoothing
    { }
    else if( strequal(current_token, "g") ) // group
    { }
    else
      printf("Unknown command '%s' in scene code at line %i: \"%s\".\n",
             current_token, line_number, current_line);
  }

  fclose(obj_file_stream);
  return 1;
}

static void obj_init_temp_storage(obj_growable_scene_data *growable_data)
{
  list_make(&growable_data->vertex_list, 10, 1);
  list_make(&growable_data->vertex_normal_list, 10, 1);
  list_make(&growable_data->vertex_texture_list, 10, 1);
  list_make(&growable_data->face_list, 10, 1);
  list_make(&growable_data->sphere_list, 10, 1);
  list_make(&growable_data->plane_list, 10, 1);
  list_make(&growable_data->light_point_list, 10, 1);
  list_make(&growable_data->light_quad_list, 10, 1);
  list_make(&growable_data->light_disc_list, 10, 1);
  list_make(&growable_data->material_list, 10, 1);	
  growable_data->camera = NULL;
}

static void obj_free_temp_storage(obj_growable_scene_data *growable_data)
{
  obj_free_half_list(&growable_data->vertex_list);
  obj_free_half_list(&growable_data->vertex_normal_list);
  obj_free_half_list(&growable_data->vertex_texture_list);
  obj_free_half_list(&growable_data->face_list);
  obj_free_half_list(&growable_data->sphere_list);
  obj_free_half_list(&growable_data->plane_list);
  obj_free_half_list(&growable_data->light_point_list);
  obj_free_half_list(&growable_data->light_quad_list);
  obj_free_half_list(&growable_data->light_disc_list);
  obj_free_half_list(&growable_data->material_list);
}

static void delete_obj_data(obj_scene_data *data_out)
{
  int i;

  for(i=0; i<data_out->vertex_count; i++)
    free(data_out->vertex_list[i]);
  free(data_out->vertex_list);
  for(i=0; i<data_out->vertex_normal_count; i++)
    free(data_out->vertex_normal_list[i]);
  free(data_out->vertex_normal_list);
  for(i=0; i<data_out->vertex_texture_count; i++)
    free(data_out->vertex_texture_list[i]);
  free(data_out->vertex_texture_list);

  for(i=0; i<data_out->face_count; i++)
    free(data_out->face_list[i]);
  free(data_out->face_list);
  for(i=0; i<data_out->sphere_count; i++)
    free(data_out->sphere_list[i]);
  free(data_out->sphere_list);
  for(i=0; i<data_out->plane_count; i++)
    free(data_out->plane_list[i]);
  free(data_out->plane_list);

  for(i=0; i<data_out->light_point_count; i++)
    free(data_out->light_point_list[i]);
  free(data_out->light_point_list);
  for(i=0; i<data_out->light_disc_count; i++)
    free(data_out->light_disc_list[i]);
  free(data_out->light_disc_list);
  for(i=0; i<data_out->light_quad_count; i++)
    free(data_out->light_quad_list[i]);
  free(data_out->light_quad_list);

  for(i=0; i<data_out->material_count; i++)
    free(data_out->material_list[i]);
  free(data_out->material_list);

  free(data_out->camera);
}

static void obj_copy_to_out_storage(obj_scene_data *data_out, obj_growable_scene_data *growable_data)
{
  data_out->vertex_count = growable_data->vertex_list.item_count;
  data_out->vertex_normal_count = growable_data->vertex_normal_list.item_count;
  data_out->vertex_texture_count = growable_data->vertex_texture_list.item_count;

  data_out->face_count = growable_data->face_list.item_count;
  data_out->sphere_count = growable_data->sphere_list.item_count;
  data_out->plane_count = growable_data->plane_list.item_count;

  data_out->light_point_count = growable_data->light_point_list.item_count;
  data_out->light_disc_count = growable_data->light_disc_list.item_count;
  data_out->light_quad_count = growable_data->light_quad_list.item_count;

  data_out->material_count = growable_data->material_list.item_count;

  data_out->vertex_list = (obj_vector**)growable_data->vertex_list.items;
  data_out->vertex_normal_list = (obj_vector**)growable_data->vertex_normal_list.items;
  data_out->vertex_texture_list = (obj_vector**)growable_data->vertex_texture_list.items;

  data_out->face_list = (obj_face**)growable_data->face_list.items;
  data_out->sphere_list = (obj_sphere**)growable_data->sphere_list.items;
  data_out->plane_list = (obj_plane**)growable_data->plane_list.items;

  data_out->light_point_list = (obj_light_point**)growable_data->light_point_list.items;
  data_out->light_disc_list = (obj_light_disc**)growable_data->light_disc_list.items;
  data_out->light_quad_list = (obj_light_quad**)growable_data->light_quad_list.items;

  data_out->material_list = (obj_material**)growable_data->material_list.items;

  data_out->camera = growable_data->camera;
}

static int parse_obj_scene(obj_scene_data *data_out, const char *filename)
{
  obj_growable_scene_data growable_data;

  obj_init_temp_storage(&growable_data);
  if (obj_parse_obj_file(&growable_data, filename) == 0)
    return 0;

  obj_copy_to_out_storage(data_out, &growable_data);
  obj_free_temp_storage(&growable_data);
  return 1;
}

objLoader::objLoader() { std::memset(this, 0, sizeof(*this)); }
objLoader::~objLoader() { delete_obj_data(&data); }

int objLoader::load(const char *filename) {
  int no_error = 1;
  no_error = parse_obj_scene(&data, filename);
  if(no_error) {
    this->vertexCount = data.vertex_count;
    this->normalCount = data.vertex_normal_count;
    this->textureCount = data.vertex_texture_count;
    this->faceCount = data.face_count;
    this->sphereCount = data.sphere_count;
    this->planeCount = data.plane_count;
    this->lightPointCount = data.light_point_count;
    this->lightDiscCount = data.light_disc_count;
    this->lightQuadCount = data.light_quad_count;
    this->materialCount = data.material_count;
    this->vertexList = data.vertex_list;
    this->normalList = data.vertex_normal_list;
    this->textureList = data.vertex_texture_list;
    this->faceList = data.face_list;
    this->sphereList = data.sphere_list;
    this->planeList = data.plane_list;
    this->lightPointList = data.light_point_list;
    this->lightDiscList = data.light_disc_list;
    this->lightQuadList = data.light_quad_list;
    this->materialList = data.material_list;
    this->camera = data.camera;
  }
  return no_error;
}

inline bool _cmp(ObjTriangle t0, ObjTriangle t1) {return t0.m < t1.m;}

static void patchName(char *str)
{
  const int sz = strlen(str);
  const int begin = str[0] == '\"' ? 1 : 0;
  const int end = str[sz-1] == '\"' ? sz-1 : sz-2;
  if (begin > end)
    return;
  if (begin == 0 && end == sz-1)
    return;
  for (int i = begin, curr = 0; i <= end; ++i, ++curr)
    str[curr] = str[i];
  str[end+1] = 0;
}

Obj::Obj(void) {std::memset(this,0,sizeof(Obj));}

INLINE uint32 str_hash(const char *key)
{
    uint32 h = 5381;
    for(size_t i = 0, k; (k = key[i]); i++) h = ((h<<5)+h)^k; // bernstein k=33 xor
    return h;
}

template <typename T>
INLINE uint32 hash(const T &elem)
{
  const char *key = (const char *) &elem;
  uint32 h = 5381;
  for(size_t i = 0; i < sizeof(T); i++) h = ((h<<5)+h)^key[i];
  return h;
}

bool
Obj::load(const char *fileName)
{
  objLoader loader;
  if (loader.load(fileName) == 0)
    return false;

  // Sort vertices and create new faces
  struct vertex_key {
    INLINE vertex_key(int p_, int n_, int t_) : p(p_), n(n_), t(t_) {}
    bool operator == (const vertex_key &other) const {
      return (p == other.p ) && (n == other.n) && (t == other.t);
    }
    bool operator < (const vertex_key &other) const {
      if (p != other.p)
        return p < other.p;
      if (n != other.n)
        return n < other.n;
      if (t != other.t)
        return t < other.t;
      return false;
    }
    int p,n,t;
  };
#if 0
  struct key_hash {
    long operator() (const vertex_key &key) const {return hash(key);}
  };
  struct key_eq {
    bool operator() (const vertex_key &x, const vertex_key &y) const {
      return (x.p == y.p) && (x.n == y.n) && (x.t == y.t);
    }
  };
  std::unordered_map<vertex_key, int, key_hash, key_eq> map;
#else
  std::map<vertex_key, int> map;

#endif
  struct poly { int v[MAX_VERTEX_COUNT]; int mat; int n; };
  std::vector<poly> polys;
  int vert_n = 0;
  for (int i = 0; i < loader.faceCount; ++i) {
    const obj_face *face = loader.faceList[i];
    int v[] = {0,0,0,0};
    for (int j = 0; j < face->vertex_count; ++j) {
      const int p = face->vertex_index[j];
      const int n = face->normal_index[j];
      const int t = face->texture_index[j];
      const vertex_key key(p,n,t);
      const auto it = map.find(key);
      if (it == map.end())
        v[j] = map[key] = vert_n++;
      else
        v[j] = it->second;
    }
    const poly p = {{v[0],v[1],v[2],v[3]},face->material_index,face->vertex_count};
    polys.push_back(p);
  }
  
  // Create triangles now
  std::vector<ObjTriangle> tris;
  for (auto poly = polys.begin(); poly != polys.end(); ++poly) {
    if (poly->n == 3) {
      const ObjTriangle tri(vec3i(poly->v[0], poly->v[1], poly->v[2]), poly->mat);
      tris.push_back(tri);
    } else {
      const ObjTriangle tri0(vec3i(poly->v[0], poly->v[1], poly->v[2]), poly->mat);
      const ObjTriangle tri1(vec3i(poly->v[0], poly->v[2], poly->v[3]), poly->mat);
      tris.push_back(tri0);
      tris.push_back(tri1);
    }
  }

  // Sort them by material and save the material group
  std::sort(tris.begin(), tris.end(), _cmp);
  std::vector<ObjMatGroup> matGrp;
  int curr = tris[0].m;
  matGrp.push_back(ObjMatGroup(0,0,curr));
  for (size_t i = 0; i < tris.size(); ++i)
    if (tris[i].m != curr) {
      curr = tris[i].m;
      matGrp.back().last = (int) (i-1);
      matGrp.push_back(ObjMatGroup((int)i,0,curr));
    }
  matGrp.back().last = tris.size() - 1;

  // Create all vertices and store them
  const size_t vertNum = map.size();
  std::vector<ObjVertex> verts;
  verts.resize(vertNum);
  for (auto it = map.begin(); it != map.end(); ++it) {
    const vertex_key src = it->first;
    const int dst = it->second;
    ObjVertex &v = verts[dst];
    const obj_vector *p = loader.vertexList[src.p];
    const obj_vector *n = loader.normalList[src.n];
    const obj_vector *t = loader.textureList[src.t];
    v.p[0] = float(p->e[0]); v.p[1] = float(p->e[1]); v.p[2] = float(p->e[2]);
    v.n[0] = float(n->e[0]); v.n[1] = float(n->e[1]); v.n[2] = float(n->e[2]);
    v.t[0] = float(t->e[0]); v.t[1] = float(t->e[1]);
  }

  // Allocate the ObjMaterial
  ObjMaterial *mat = new ObjMaterial[loader.materialCount];
  std::memset(mat, 0, sizeof(ObjMaterial) * loader.materialCount);

#define COPY_FIELD(NAME)                                \
  if (from.NAME) {                                      \
    const size_t len = std::strlen(from.NAME);          \
    to.NAME = new char[len + 1];                        \
    memcpy(to.NAME, from.NAME, std::strlen(from.NAME)); \
    to.NAME[len] = 0;                                   \
    patchName(to.NAME);                                 \
  }
  for (int i = 0; i < loader.materialCount; ++i) {
    const obj_material &from = *loader.materialList[i];
    ObjMaterial &to = mat[i];
    COPY_FIELD(name);
    COPY_FIELD(map_Ka);
    COPY_FIELD(map_Kd);
    COPY_FIELD(map_D);
    COPY_FIELD(map_Bump);
  }
#undef COPY_FIELD

  // Now return the properly allocated Obj
  std::memset(this, 0, sizeof(Obj));
  this->triNum = tris.size();
  this->vertNum = verts.size();
  this->grpNum = matGrp.size();
  this->matNum = loader.materialCount;
  if (this->triNum) {
    this->tri = new ObjTriangle[this->triNum];
    std::memcpy(this->tri, &tris[0],  sizeof(ObjTriangle) * this->triNum);
  }
  if (this->vertNum) {
    this->vert = new ObjVertex[this->vertNum];
    std::memcpy(this->vert, &verts[0], sizeof(ObjVertex) * this->vertNum);
  }
  if (this->grpNum) {
    this->grp = new ObjMatGroup[this->grpNum];
    std::memcpy(this->grp,  &matGrp[0],  sizeof(ObjMatGroup) * this->grpNum);
  }
  this->mat = mat;
  return true;
}

Obj::~Obj(void)
{
  SAFE_DELETE_ARRAY(this->tri);
  SAFE_DELETE_ARRAY(this->vert);
  SAFE_DELETE_ARRAY(this->grp);
  for (size_t i = 0; i < this->matNum; ++i) {
    ObjMaterial &mat = this->mat[i];
    SAFE_DELETE_ARRAY(mat.name);
    SAFE_DELETE_ARRAY(mat.map_Ka);
    SAFE_DELETE_ARRAY(mat.map_Kd);
    SAFE_DELETE_ARRAY(mat.map_D);
    SAFE_DELETE_ARRAY(mat.map_Bump);
  }
  SAFE_DELETE_ARRAY(this->mat);
}

