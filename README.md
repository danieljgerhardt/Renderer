This is my personal renderer project. It was initially built off a DX12 base I wrote for a project called Breakpoint. I have gutted the "Breakpoint" part of the project and built a renderer out of it, to keep my programming and graphics knowledge up and have some fun.

List of goals:
- [x] Remove Breakpoint files
- [x] Rename sln
- [x] Rework scene organization to actually make sense
- [x] Add GLTF importing
- [x] Add texture/material support
- [x] Add resource manager
- [x] Refactor main + ImGUI for neater code
- [ ] DGR namespace wrapper
- [ ] shaders + pipelines owned by resource manager
- [ ] improve scene vs pipeline vs drawable abstractions to allow easier swapping
- [ ] post processing
- [ ] Math abstraction class
- [ ] More info in ImGUI(num passes, num resources, num triangles)
- [ ] Real time scene loading GUI
- [ ] environment mapping
- [ ] rasterized PBR support
- [ ] skinning
- [ ] path tracing with DXR
- [ ] Use render graph
- [ ] Support gaussian splats
- [ ] SSR and deferred rendering for rasterized PBR
- [ ] Support multiple lights with forward, clustered, etc.
