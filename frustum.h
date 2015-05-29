
#ifndef FRUSTUM_H
#define FRUSTUM_H

class Frustum
{
    vector<Plane> planes;

    vector<int>   sizes;
 public:
    void push_portal(class Portal *p, sgVec3 vp);
    void pop_portal();

    void push_plane(Plane &p);
    void pop_planes(int n);

    void setup_from_matrix(sgMat4);

    bool cull_face(Face *f);
    bool cull_sphere(sgVec3 center, float radius);

    int get_nr_planes() { return planes.size(); }
};

#endif
