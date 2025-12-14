> [!NOTE]  
> This info might be outdated.

First byte:
- 0 - Amount of types (1-8, write as 0-7, 3 bits)
- 3 - Name
- 4 - Layer
- 5, 6 - Geometry type (point = 00, line = 01, area = 10)
- 7 - Bit indicating the presence of additional information:
  - Point - rank (1 byte as the logarithm of population base 1.1);
  - Line - road number (string);
  - Area - house number (string, optimized for storing a two-digit number);

* Write types, name, layer, additional information, and point (for point type)

1 or 2 bytes of the next header (only for line and/or area objects):
* 4 bits for the number of internal points for a line object:
  - 0 - geometry is extracted; read the offset mask and offsets;
  - 2 - 0 bytes for the simplification mask;
  - 3-6 - 1 byte for the simplification mask;
  - 7-10 - 2 simplification mask bytes;
  - 11-14 - 3 simplification mask bytes;
* 4 bits for the number of internal triangles for an area object:
  - 0 - geometry is extracted; read the offset mask and offsets;
  - \>0 - number of triangles in one strip (for multiple strips, geometry is extracted);

* 4 bits for the offset mask for line and area objects.
The offset mask determines the presence of extracted geometry for the i-th scale row (out of 4, according to the corresponding bit).

These 2 bytes may be located in one byte when the object is of one type or the geometry is not extracted.
In reality, this will be 2 bytes when the object is both line and area and has extracted geometry.

Following bytes:
* Write geometry ...
  - Simplification mask for a line object (1-3 bytes):
    The 1-byte simplification mask encodes the visibility of 4 points in 4 scale rows (2 bits per point), i.e.
    equal to the scale row value from which the point is already visible.
  - Array of geometry points (triangle strip) according to the known amount VarInt64
* ... or write the array of offsets to the extracted geometry (number taken from the offset mask)

Extracted geometry for a scale is representing a block:
- Size of the geometry in bytes
- Serialized VarInt64s by the number of bytes

For a line object, they represent an array of points.
For an area object, they represent the following sequences:
- Number of points on the strip
- The strip itself (array of points)
