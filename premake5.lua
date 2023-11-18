-- premake5.lua
workspace "RTRayTracer"
   architecture "x64"
   configurations { "Debug", "Release", "Dist" }
   startproject "RTRayTracer"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
include "Walnut/WalnutExternal.lua"

include "RTRayTracer"