# Flan Collision Model
## File header
| Type    | Name       | Description                                   |
| ------- | ---------- | --------------------------------------------- |
| char[4] | file_magic | File magic: "FVIS"                            |
| u32     | offset_vis_bvh | Offset into the binary section to the start of the serialized BVH | 
| u32     | offset_vis_lists | Offset into the binary section to the start of the visibility bitfield array | 

## Visibility BVH
The visibility list is stored as a Bounding Volume Hierarchy. The volumes are based on the collision model, so all the spots where the player is expected to be able to go are accounted for. To figure out which level geometry segments the player can see, perform a player intersection with the BVH to figure out which leaf node(s) the player is touching.

Nodes are stored contiguously as one big array. The array does not need to specify the length, since a properly formed BVH stops at the leaf nodes.

### Nodes
| Type    | Name       | Description                                   |
| ------- | ---------- | --------------------------------------------- |
| int16_t | min_x | Bounding box minimum X coordinate                           |
| int16_t | min_y | Bounding box minimum Y coordinate                           |
| int16_t | min_z | Bounding box minimum Z coordinate                           |
| int16_t | max_x | Bounding box maximum X coordinate                           |
| int16_t | max_y | Bounding box maximum Y coordinate                           |
| int16_t | max_z | Bounding box maximum Z coordinate                           |
| uint32_t | child_or_vis_index | If the highest bit is set, the lower 31 bits represent an index into the `vis_lists` array. Otherwise, this represents the first child index, where the second child is child_or_vis_index + 1.


## Visibility List
| Type | Name             | Description                                                                           |
| ---- | ---------------- | ------------------------------------------------------------------------------------- |
| u128 | visible_sections | Bitfield of 128 sections, where 0 means not visible, and 1 means visible. Little Endian. |