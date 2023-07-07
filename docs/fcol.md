# Flan Collision Model
## File header
| Type    | Name       | Description                                   |
| ------- | ---------- | --------------------------------------------- |
| char[4] | file_magic | File magic: "FCOL"                            |
| u32     | n_verts    | The number of vertices in this collision mesh | 

## Collision Vertex
| Type | Name               | Description                   |
| ---- | ------------------ | ----------------------------- |
| i16  | pos_x              | X-position of the vertex      |
| i16  | pos_y              | Y-position of the vertex      |
| i16  | pos_z              | Z-position of the vertex      |
| u16  | terrain_id         | What type of terrain it is.   |