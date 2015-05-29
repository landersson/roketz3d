
#ifndef AABSP_H
#define AABSP_H

#include <plib/sg.h>

struct FTreeNode
{
    bool                is_leaf; 

    // split plane normal direction (0 = (1,0,0), 1=(0,1,0)...)
    int                major;    

    // used for storing face pointers when constructing the tree
    vector<Face *>      faces;

    // position of split plane along split plane normal
    float               split_pos;

    // left right subtrees
    struct FTreeNode    *neg_branch, *pos_branch;
    
    // first index of this node's faces in global face list
    int                 first, nr_faces;
};


class AABSP
{
    FTreeNode           *face_tree_root;

    int split_box(FTreeNode *node, const sgVec3 min, 
                  const sgVec3 max, int depth);

    void collect_touching_faces_rek(FTreeNode *node, const sgVec3 center,
                                    const sgVec3 side_lengths, 
                                    vector<vector<Face *> *> &intervals);
//                                       vector<int> &intervals);

 public:
    void create_face_tree(vector<Room*> &rooms, unsigned nr_faces);

    void collect_touching_faces(const sgVec3 center,
                                const sgVec3 side_lengths, 
                                vector<vector<Face *> *> &intervals);

    void collide_and_slide(sgVec3 pos, sgVec3 vel, float radius, 
                           sgVec3 new_pos, int depth, int last_slided_face);

    bool collide_and_bounce(const sgVec3 pos, const sgVec3 vel, 
                            float radius, float ticks,  
                            sgVec3 new_pos, sgVec3 new_vel);

    bool collide(const sgVec3 pos, const sgVec3 vel, float radius, float ticks,  
                 sgVec3 hit_pos);
};

#endif
