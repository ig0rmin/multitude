project ("Poetic")
  kind "SharedLib"
  defines {"POETIC_EXPORT"}
  files  { "**.hpp", "**.cpp" }
  includedirs { "." }
  
  links{"Radiant", "Nimble", "Luminous", "Valuable"}
 
  -- OpenGL 

  -- Freetype
  configuration { "windows" }
    links ("freetype246")
  configuration { "windows", "x64" }
    includedirs { "../Win64x/include/freetype2" }
  configuration { "windows", "x32" }
    includedirs { "../Win32x/include/freetype2" }
  configuration { "linux"}
    includedirs { "/usr/include/freetype2/" }
  configuration {}
