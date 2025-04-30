<h1>Space Invaders</h1>
<p><b>Summary: </b>A recreation of the classic Space Invaders game created in C++. </p>
<p><b>Dependencies: </b>cstudio, cstdint, GL/glew.h, GLFW/glfw3.h, chrono, thread, random</p>
<h2>Project Elements</h2>
<ul>
  <li>
    <h4>Window</h4>
    <p><b>Description: </b>The window is created using GLFW and OpenGL 3.3. It uses glViewport to dynimically scale to any size.  </p>    
  </li>
  <li>
    <h4>Buffer</h4>
    <p><b>Description: </b>A CPU-side Buffer struct stores the game screen as a 2D array of pixels (uint32_t RGBA format). glTexSubImage2D uploads this buffer to the GPU to update the texture shown in the window</p>    
  </li>
  <li>
    <h4>Sprites & Animations</h4>
    <p><b>Description: </b>Sprites for aliens, the player, text, and bullets are defined as bitmaps using a Sprite struct. Animation is handled through SpriteAnimation, which swaps between sprite frames at a fixed duration.</p>    
  </li>
  <li>
    <h4>Shaders</h4>
    <p><b>Description: </b>A vertex shader and fragment shader are used to render the game buffer as a fullscreen triangle, stretching the CPU-rendered buffer texture across the OpenGL viewport. </p>    
  </li>
  <li>
    <h4>Game Mechanics</h4>
    <p><b>Description: </b>The aliens are able to move left and right, shift down at the borders, and shoot at the player. The player is also able to move left and right as well as shoot. Collisions are handled by bounding box overlap checks</p>    
  </li>
  <li>
    <h4>Frame Rate Lock</h4>
    <p><b>Description: </b> Frame timing is rendered with std::chrono to cap the game at 60fps. Each frame's duration is measured and if it was less than 1/60 of a second the process sleeps the difference. </p>    
  </li>
</ul>
