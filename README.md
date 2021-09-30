# Glacier Prim IO
 GUI Tool for import and export of Glacier 2 Prim resources from and to glTF.

# How to use:

This tool is in active development and there is currently no comprehensive tutorial but here are a few short pointers to get started:
### Export:
To export a model you need it's runtime id. There is currently no good way to find the runtime id of a model directly. The easiest way to find the runtime id of a specific model is to search [this list](https://gist.github.com/pawREP/cd948ee8882f3e1d218a481f0167fd8f) which contains material names and all associated PRIM runtime ids. If you are looking for hero disguises specifically, check out [HMBM47's](https://github.com/HMBM47) excellent [list of disguises](https://gist.github.com/pawREP/50dc81c2dd09493f48ba60e37a00819a).
Once you found the id of the model you want to export, type it into the prim id field, select an export directory and click export. 
### Import:
A few tips and pointers about importing meshes:
 - Don't move any exported files relative to each other or rename them if you expect them to be imported again.
 - Don't rename any meshes inside the glTF file. Keep in mind that format conversion tools might rename meshes automatically. Make sure the names are correct before trying to import a glTF.
 - Don't make any changes to skeletons you exported from the game. Skeletons don't get reimported, so changes are pointless. The skeleton should only be used to skin models.
 - You can delete individual meshes, for example to get rid of lod models. If you do so, select the max LOD range option when importing so the imported model covers all LOD ranges.
 - When exporting a glTF from Blender, select the `gltf + bin + texture` format. Embedded texture or `.glb` files are not supported. 
 - The material in the glTF doesn't get imported, the importer will only reimport the tga textures that where generated during export. You can change the textures of cource but make sure the changed textures have the same dimensions as the original.
 
### Textures:
Meshes will most commonly use three texture maps, albedo, normal and a metallic/roughness map. The first two should be self-explanatory, the metallic/roughness map contains the metallicity in the color channel (more white = less rough) and the roughness in the alpha channel (more white = more metallic).
More complicated models may contain a number of additional textures. 

# Troubleshooting:
 - If the lighting of imported models is messed up you can try to play around with the `Invert Normals` options. If this doesn't fix it you have to change the orientation of the coordinates of your model in the editor. Try to match the transformation of the original model.
 - If you get any other unspecified error during import, delete all user generated rpkg patch files from the Runtime directory, restart the I/O tool and try again. 
 - If you have any other specific issues or questions, you can reach me on the [Hitman Modding discord](https://discord.gg/6UDtuYhZP6)
