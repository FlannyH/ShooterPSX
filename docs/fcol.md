# Flan Collision Model
## File header
| Type    | Name                 | Description                                                  |
| ------- | -------------------- | ------------------------------------------------------------ |
| char[4] | file_magic           | File magic: "FCOL"                                           |
| u32     | n_verts              | The number of vertices in this collision mesh                |
| u32     | n_nodes              | The number of nodes in the collision mesh's BVH              |
| u32     | triangle_data_offset | Offset to raw triangle data                                  |
| u32     | terrain_id_offset    | Offset to array of 8 bit terrain IDs for each triangle       |
| u32     | bvh_nodes_offset     | Offset to the precalculated BVH's node pool                  |
| u32     | bvh_indices_offset   | Offset to the precalculated BVH's index array                |
| u32     | nav_graph_offset     | Offset to the precalculated navigation graph for the enemies |

All offsets are relative to the start of the binary section, which is located right after the header.

The first node in the BVH node pool is always the root.

## Collision Triangle
| Type   | Name   | Description                                |
| ------ | ------ | ------------------------------------------ |
| i32[3] | v0     | Vertex 0, in collision space               |
| i32[3] | v1     | Vertex 1, in collision space               |
| i32[3] | v2     | Vertex 2, in collision space               |
| i32[3] | normal | Triangle's normal vector in graphics space |

## BVH Node
| Type   | Name            | Description                                                                                                               |
| ------ | --------------- | ------------------------------------------------------------------------------------------------------------------------- |
| i32[3] | bounds_min      | Axis aligned bounding box around all primitives inside this node, in collision space                                      |
| i32[3] | bounds_max      | Axis aligned bounding box around all primitives inside this node, in collision space                                      |
| u16    | left_first      | If this is a leaf, this is the index of the first primitive, otherwise, this is the index of the first of two child nodes |
| u16    | primitive_count | If this value is 0, this is a leaf node                                                                                   |

## Navigation Graph
### Header
| Type          | Name        | Description                |
| ------------- | ----------- | -------------------------- |
| u16           | n_nav_nodes | Number of navigation nodes |
| nav_node_t[n] | nodes       | All navigation graph nodes |

### Node
| Type   | Name      | Description                                 |
| ------ | --------- | ------------------------------------------- |
| i16[3] | position  | Position of the node, in model space        |
| u16[4] | neighbors | The closest accessible nodes from this node |