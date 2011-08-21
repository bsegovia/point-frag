#include "pointcloud.hpp"
#include "models/obj.hpp"
#include "math/distribution.hpp"
#include "math/random.hpp"
#include "math/matrix.hpp"
#include "sys/tasking.hpp"
#include "rtcore/rtcore.hpp"

#include <vector>
#include <deque>

namespace pf
{
  struct LayerDepthImage {
    INLINE LayerDepthImage(uint32_t w_, uint32_t h_) : 
       texel(new vec2i[w_*h_]), w(w_), h(h_)
    {}
    INLINE ~LayerDepthImage(void) {
      delete [] texel;
    }
    INLINE vec2i& get(uint32_t x, uint32_t y) {
      return texel[x+y*w];
    }
    INLINE Point* point(uint32_t x, uint32_t y) {
      return &pt[texel[x+y*w].x];
    }
    INLINE int32_t pointNum(uint32_t x, uint32_t y) const {
      return texel[x+y*w].y;
    }
    vec2i *texel;
    std::vector<Point> pt;
    uint32_t w, h;
  };

  struct PointOctree {
    PointOctree(BBox3f box_, vec3i dim_, float step_) :
      box(box_), dim(dim_), step(step_) {nodes.push_back(zero);}
    /*! Compute the integer position of the point */
    INLINE vec3i quantize(vec3f p) {
      return vec3i((p - box.lower) / step);
    }
    /*! Insert a point in the octree structure */
    void insert(const Point &p);
    /*! Make a breadth first allocation of the tree and remove empty nodes */
    void reallocate(void);
    /*! Top down mip-mapping: bottom elements merged into upper ones */
    void mipmap(void);

    std::vector<Point> nodes; /*! Root of the tree is at index 0 */
    BBox3f box;               /*! Bounding box of the scene */
    vec3i dim;                /*! Dimension of the octree */
    float step;               /*! Quantization distance */
  };

  static INLINE uint32_t getChildID(vec3i p) {
    uint32_t which = uint32_t(p.x) >> 31;
    which |= (uint32_t(p.y) >> 31) << 1;
    which |= (uint32_t(p.z) >> 31) << 2;
    return which;
  }

  void PointOctree::insert(const Point &p)
  {
    const vec3i pi = this->quantize(p.p);
    vec3i lower = zero, upper = this->dim;
    uint32_t curr = 0;

    // Look for the leaf first
    for (;;) {
      const vec3i delta = (upper - lower) / 2;
      if (delta.x + delta.y + delta.z == 0)
        break;

      // Allocate children if not already done
      if (this->nodes[curr].child == 0) {
        this->nodes[curr].child = this->nodes.size();
        for (int i = 0; i < 8; ++i)
          this->nodes.push_back(zero);
      }

      // Process the next node
      const vec3i middle = (upper+lower) / 2;
      curr = this->nodes[curr].child + getChildID(pi-middle);
      lower.x = middle.x <= pi.x ? middle.x : lower.x;
      lower.y = middle.y <= pi.y ? middle.y : lower.y;
      lower.z = middle.z <= pi.z ? middle.z : lower.z;
      upper.x = middle.x >  pi.x ? middle.x : upper.x;
      upper.y = middle.y >  pi.y ? middle.y : upper.y;
      upper.z = middle.z >  pi.z ? middle.z : upper.z;
    }

    // Set the point here
    this->nodes[curr] = p;
    this->nodes[curr].n[3]++; // number of points set here (0 to 3)
  }

  void PointOctree::reallocate(void)
  {
    // Just store node index before and after reallocation
    struct Remap {
      Remap(uint32_t before_, uint32_t after_) :
        before(before_), after(after_) {}
      uint32_t before, after;
    };
    std::vector<Point> dst;  // Properply allocated and decimated
    std::deque<Remap> remap; // for breadth first traversal

    // Start with the root
    remap.push_back(Remap(0,0));
    dst.push_back(this->nodes[0]);
    for (;;) {
      if (remap.empty())
        break;
      const Remap r = remap[0];
      const Point before = this->nodes[r.before];
      remap.pop_front();
      // Leaf, we are done
      if (before.child == 0)
        continue;
      // Non-leaf look for children
      dst[r.after].child = dst.size();
      dst[r.after].n[3] = 0; // child number here
      dst[r.after].t[3] = 0; // child mask here
      for (int i = 0; i < 8; ++i) {
        const uint32_t childID = before.child + i;
        const Point child = this->nodes[childID];
        // This is a leaf or it has children
        if (child.n[3] > 0 || child.child != 0) {
          remap.push_back(Remap(childID, dst.size()));
          dst[r.after].n[3]++;
          dst[r.after].t[3] |= 1 << i;
          dst.push_back(child);
        }
      }
    }
    this->nodes = dst;
  }

  void PointOctree::mipmap()
  {
    enum { MAX_STACK_DEPTH = 256 };
    uint32_t nodeID[MAX_STACK_DEPTH];
    uint32_t depth = 1;
    nodeID[0] = 0; // root
    for (;;) {
      if (depth == 0)
        break;
      const uint32_t curr = nodeID[--depth];
      Point &node = this->nodes[curr];
      // Leaf, nothing to do
      if (node.child == 0)
        continue;
      else if ((node.child & (1<<31)) == 0) { 
        ++depth;                                  // Repush the node
        uint32_t childID = node.child, curr = 0;  // Store the current childID
        node.child |= 1 << 31;                    // Mark that we explored it
        for (int i = 0; i < 8; ++i)
          if (node.t[3] & (1<<i)) {
            nodeID[depth++] = childID+curr;
            curr++;
          }
      }
      // Second time we pass on it
      else {
        vec3f n(zero), c(zero);
        uint32_t curr = 0;
        node.child &= ~(1<<31); // Clear the flag we set in the first pass
        for (int i = 0; i < 8; ++i)
          if (node.t[3] & (1<<i)) {
            const Point child = this->nodes[node.child+curr];
            n += child.getNormal();
            c += child.getColor();
            curr++;
          }
        const mat3x3f m(n);
        node.setNormal(m.vy);
        node.setTangent(m.vx);
        node.setColor(c / float(curr));
      }
    }
  }

  Point* buildPointCloud(const Obj &mesh, size_t &pointNum, float density)
  {
    BBox3f box(empty);
    TaskScheduler::init();
    std::vector<BuildTriangle> triangles;
    for (size_t i = 0; i < mesh.triNum; ++i) {
      const ObjTriangle &tri = mesh.tri[i];
      const ObjVertex &ov0 = mesh.vert[tri.v[0]];
      const ObjVertex &ov1 = mesh.vert[tri.v[1]];
      const ObjVertex &ov2 = mesh.vert[tri.v[2]];
      box.grow(ov0.p); box.grow(ov1.p); box.grow(ov2.p);
      triangles.push_back(BuildTriangle(ov0.p, ov1.p, ov2.p, i, i));
    }

    // Build the intersector
    Intersector *isec = rtcCreateAccel("bvh2", &triangles[0], mesh.triNum);

    // Grow the box to avoid self-collision
    const float seg = sqrt(1.f / density);
    box.lower -= vec3f(seg);

    // Iterate over all pixels
    pointNum = 0;
    LayerDepthImage *image[3];
    const vec3i size((box.upper - box.lower) / seg);

    for (uint32_t dim0 = 0; dim0 < 3; ++dim0) {
      const uint32_t dim1 = (dim0 + 1) % 3, dim2 = (dim0 + 2) % 3;
      image[dim0] = new LayerDepthImage(size[dim1], size[dim2]);
      for (int r = 0; r < size[dim1]; ++r)
      for (int s = 0; s < size[dim2]; ++s) {
        // Initially there is no point for this coordinate
        vec2i &texel = image[dim0]->get(r,s);
        texel.x = image[dim0]->pt.size();
        // Cast a ray and find all intersections along it
        vec3f org, dir;
        org[dim0] = box.lower[dim0];
        org[dim1] = box.lower[dim1] + float(r) * seg;
        org[dim2] = box.lower[dim2] + float(s) * seg;
        org += vec3f(seg) / 2.f;
        dir[dim0] = 1.f;
        dir[dim1] = 0.f;
        dir[dim2] = 0.f;
        Ray ray(org, dir);
        for (;;) {
          Hit hit;
          isec->intersect(ray, hit);
          if (hit == false)
            break;
          const size_t triID = hit.id0;
          const int *IDs = &mesh.tri[triID].v[0];
          const ObjVertex a = mesh.vert[IDs[0]];
          const ObjVertex b = mesh.vert[IDs[1]];
          const ObjVertex c = mesh.vert[IDs[2]];
          const float u = hit.u, v = hit.v, w = 1.f - hit.u - hit.v;
          const vec3f n = normalize(u*a.n + v*b.n + w*c.n);
          const mat3x3f m(n);
          // Set point property
          Point p;
          p.child = 0;
          p.p = u*a.p + v*b.p + w*c.p;
          p.setNormal(n);
          p.setTangent(m.vx);
          p.c[0] = p.c[1] = p.c[2] = 255;
          image[dim0]->pt.push_back(p);
          texel.y++;
          // Try to find a next point
          ray.near = hit.t + seg;
        }
      }
    }
#if 0
    pointNum = image[0]->pt.size() + image[1]->pt.size() + image[2]->pt.size();
    std::cout << "pointNum: " << pointNum << std::endl;
    Point *dst0 = new Point[pointNum];
    Point *dst1 = dst0 + image[0]->pt.size();
    Point *dst2 = dst1 + image[1]->pt.size();
    std::memcpy(dst0, &image[0]->pt[0], sizeof(Point) * image[0]->pt.size());
    std::memcpy(dst1, &image[1]->pt[0], sizeof(Point) * image[1]->pt.size());
    std::memcpy(dst2, &image[2]->pt[0], sizeof(Point) * image[2]->pt.size());
    return dst0;
#else
    // Insert all points (one by one) in the octree
    PointOctree octree(box, size, seg);
    for (size_t dim = 0; dim < 3; ++dim) {
      for (size_t i = 0; i < image[dim]->pt.size(); ++i)
        octree.insert(image[dim]->pt[i]);
      delete image[dim];
    }
    octree.reallocate();
    octree.mipmap();
    pointNum = octree.nodes.size();
    std::cout << "pointNum: " << pointNum << std::endl;
    Point *dst = new Point[pointNum];
    std::memcpy(dst, &octree.nodes[0], pointNum * sizeof(Point));
    return dst;
#endif
  }

}

