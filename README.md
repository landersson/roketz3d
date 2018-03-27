
Roketz3D:

   This game is supposed to be a 3d version of the good old gravity /
   thrust based games which has been around more or less since the
   birth of home computers. When it comes to the controls, going from
   two to three dimensions adds quite a bit of complexity. We have
   tried to come up with a reasonable control system, but the result
   is... hmmm, less than intuitive. After hours of practice though,
   it's possible to fly pretty well, and the feeling is quite
   nice. And after all, who said flying a rocket should be easy? :)
   
   https://vimeo.com/56525162


Controls:

   Mouse  - Control rocket orientation... Up/down rotates the rocket
            from -180 to 180 degrees around the x-axis, left/right
            rotates around the y-axis.
            Takes some (or rather alot of) practice.

   Space  - toggle rocket engine
   LMB    - fire plasma gun
   f      - toggle fullscreen (only under X11)
   r      - toggle rocket/debug camera mode
   c      - toggle cubemapping
   m      - toggle music
   p      - pause
   q      - quit
 
   F12    - Save screenshots as "roketz000.bmp", "roketz001.bmp"...
            (overwrites old ones)


   The top left indicator shows the amount of fuel left, and the right
   one the amount of bumper field generator energy remaining. When
   either of these starts to look a bit limited it's a good idea to
   return to the base for refueling and service as soon as possible.


Stuff implemented:

 * Descent3 D3L level file loader. The source code of a Descent3 level
   editor is available for download. The source code is well
   documented and it was pretty easy to figure out how the level data
   is stored in D3L files. The data is very suitable for use with a
   portal based indoor 3d engine. Using existing tools we have also
   extracted useful textures and animations from our Descent3 data
   files since creating this ourselves would just take too much
   time. The titlescreen, status indicators etc has been created by
   Christofer though. The level we use is based on an existing level,
   but has been tweaked for rocket flights.

 * Wavefront OBJ loader. Very limited and just picks out the stuff we
   use. Materials are hardcoded. The asteroids have been created using
   the Wings3D modeller, and the remaining objects picked up on the
   internet.

 * Bounding sphere frustum culling. Only objects and portal polygons
   are culled. Room polygons are drawn as display lists and are not
   individually culled.
 
 * Portal culling. When encountering a portal polygon the current
   frustum is complemented by a set of clippling planes made up from
   the current viewpoint and the polygon edges. The room on the other
   side of the portal is then drawn, and any portals found are culled
   to the current frustum and traversed recursively in the same manner
   if visible. All objects belong to a single room, and all portals in
   the current room are checked when the objects moves to see if it
   has passed into another room.

 * Static world lighting. The D3L level files contain lightmap
   textures for the static world, calculated using radiosity by the
   editor. When rendering a new frame, the lightmaps are the first
   thing drawn. After this dynamic lights are added, and finally the
   actual wall textures are blended on top of it all.

 * Axis aligned BSP tree. Used to quickly locate the polygons that
   needs to be considered for collision detection and dynamic
   lighting. Created dynamically during initialization.

 * Dynamic world lighting. A circular light texture is mapped to the
   wall polygons to simulate dynamic lighting. The advantage of this
   method is that we can use large polygons for walls and stuff, and
   still get fine grained lighing. For object lighting a set of
   "light" objects representing normal OpenGL point light sources are
   placed in each room and used to light the objects in that room. A
   problem with this is that we get an ugly "snap" of lighting when an
   object passes from one room to another. Careful placement of the
   light objects may improve this, but a better way would probably to
   put the lights in some spatial structure and always draw the lights
   that are closest to each object.

 * Collision detection and response. We implement sphere-polygon and
   sphere-sphere collision detected. As for object-object collision,
   all objects in the same room are tested against each other. Quick
   rejection tests make this quite fast, at least for the low number
   of objects we use. Object-wall collision is not restricted to room
   boundaries but use the AABSP-tree to locate which polygons needs to
   be collided. Object-object collision use simple spherical collision
   response where elasticity and objects masses are considered.

 * Billboards. Always faces the viewer and can display texture
   animations. Used for explosions, jet flares etc.

 * Particle systems. Quite basic, but works for what we need. Should be
   made more general and optimized using vertex arrays or something
   similar.

 * Camera movement. During normal flight the camera tries to put
   itself 50 distance units behind the rocket, in the opposite
   direction of the current velocity vector, and locking straigt
   towards the rocket. To get more smooth movement when the velocity
   gradient is large (i.e during wall collisions) the camera moves
   gradually from it's current location to the wanted location 50
   units behind the rocket. This works quite well and it looks like
   we can do without any camera-wall collision detection.

 * Dynamic cube mapping. Uses the "GL_ARB_texture_cube_map" OpenGL
   extension to map the environment onto the rocket. The cube map is
   dynamically updated by rerendering one of it's 6 faces each frame,
   which seems often enough unless the frame rate is very low.


Source code:

 * Uses SDL/SDL_image/SDL_mixer for portable bitmap loading, audio and
   OpenGL initialization.
   http://www.libsdl.org

 * Uses plib/sg for basic vector and matrix math.
   http://plib.sourceforge.net/sg/index.html.

 * All code has been written completely from scratch, except the
   sphere / edge collision function which is based on code found on
   the internet.  
   http://www.gdmag.com/code.htm, aug01.zip

 * Currently a terrible mess! Should be seriosly cleaned up before
   doing anything else.


