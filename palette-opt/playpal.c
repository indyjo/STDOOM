// array size is 768
const unsigned char PLAYPAL[]  = {
  0x00, 0x00, 0x00, 0x1f, 0x17, 0x0b, 0x17, 0x0f, 0x07, 0x4b, 0x4b, 0x4b, 0xff, 0xff, 0xff, 0x1b, 
  0x1b, 0x1b, 0x13, 0x13, 0x13, 0x0b, 0x0b, 0x0b, 0x07, 0x07, 0x07, 0x2f, 0x37, 0x1f, 0x23, 0x2b, 
  0x0f, 0x17, 0x1f, 0x07, 0x0f, 0x17, 0x00, 0x4f, 0x3b, 0x2b, 0x47, 0x33, 0x23, 0x3f, 0x2b, 0x1b, 
  0xff, 0xb7, 0xb7, 0xf7, 0xab, 0xab, 0xf3, 0xa3, 0xa3, 0xeb, 0x97, 0x97, 0xe7, 0x8f, 0x8f, 0xdf, 
  0x87, 0x87, 0xdb, 0x7b, 0x7b, 0xd3, 0x73, 0x73, 0xcb, 0x6b, 0x6b, 0xc7, 0x63, 0x63, 0xbf, 0x5b, 
  0x5b, 0xbb, 0x57, 0x57, 0xb3, 0x4f, 0x4f, 0xaf, 0x47, 0x47, 0xa7, 0x3f, 0x3f, 0xa3, 0x3b, 0x3b, 
  0x9b, 0x33, 0x33, 0x97, 0x2f, 0x2f, 0x8f, 0x2b, 0x2b, 0x8b, 0x23, 0x23, 0x83, 0x1f, 0x1f, 0x7f, 
  0x1b, 0x1b, 0x77, 0x17, 0x17, 0x73, 0x13, 0x13, 0x6b, 0x0f, 0x0f, 0x67, 0x0b, 0x0b, 0x5f, 0x07, 
  0x07, 0x5b, 0x07, 0x07, 0x53, 0x07, 0x07, 0x4f, 0x00, 0x00, 0x47, 0x00, 0x00, 0x43, 0x00, 0x00, 
  0xff, 0xeb, 0xdf, 0xff, 0xe3, 0xd3, 0xff, 0xdb, 0xc7, 0xff, 0xd3, 0xbb, 0xff, 0xcf, 0xb3, 0xff, 
  0xc7, 0xa7, 0xff, 0xbf, 0x9b, 0xff, 0xbb, 0x93, 0xff, 0xb3, 0x83, 0xf7, 0xab, 0x7b, 0xef, 0xa3, 
  0x73, 0xe7, 0x9b, 0x6b, 0xdf, 0x93, 0x63, 0xd7, 0x8b, 0x5b, 0xcf, 0x83, 0x53, 0xcb, 0x7f, 0x4f, 
  0xbf, 0x7b, 0x4b, 0xb3, 0x73, 0x47, 0xab, 0x6f, 0x43, 0xa3, 0x6b, 0x3f, 0x9b, 0x63, 0x3b, 0x8f, 
  0x5f, 0x37, 0x87, 0x57, 0x33, 0x7f, 0x53, 0x2f, 0x77, 0x4f, 0x2b, 0x6b, 0x47, 0x27, 0x5f, 0x43, 
  0x23, 0x53, 0x3f, 0x1f, 0x4b, 0x37, 0x1b, 0x3f, 0x2f, 0x17, 0x33, 0x2b, 0x13, 0x2b, 0x23, 0x0f, 
  0xef, 0xef, 0xef, 0xe7, 0xe7, 0xe7, 0xdf, 0xdf, 0xdf, 0xdb, 0xdb, 0xdb, 0xd3, 0xd3, 0xd3, 0xcb, 
  0xcb, 0xcb, 0xc7, 0xc7, 0xc7, 0xbf, 0xbf, 0xbf, 0xb7, 0xb7, 0xb7, 0xb3, 0xb3, 0xb3, 0xab, 0xab, 
  0xab, 0xa7, 0xa7, 0xa7, 0x9f, 0x9f, 0x9f, 0x97, 0x97, 0x97, 0x93, 0x93, 0x93, 0x8b, 0x8b, 0x8b, 
  0x83, 0x83, 0x83, 0x7f, 0x7f, 0x7f, 0x77, 0x77, 0x77, 0x6f, 0x6f, 0x6f, 0x6b, 0x6b, 0x6b, 0x63, 
  0x63, 0x63, 0x5b, 0x5b, 0x5b, 0x57, 0x57, 0x57, 0x4f, 0x4f, 0x4f, 0x47, 0x47, 0x47, 0x43, 0x43, 
  0x43, 0x3b, 0x3b, 0x3b, 0x37, 0x37, 0x37, 0x2f, 0x2f, 0x2f, 0x27, 0x27, 0x27, 0x23, 0x23, 0x23, 
  0x77, 0xff, 0x6f, 0x6f, 0xef, 0x67, 0x67, 0xdf, 0x5f, 0x5f, 0xcf, 0x57, 0x5b, 0xbf, 0x4f, 0x53, 
  0xaf, 0x47, 0x4b, 0x9f, 0x3f, 0x43, 0x93, 0x37, 0x3f, 0x83, 0x2f, 0x37, 0x73, 0x2b, 0x2f, 0x63, 
  0x23, 0x27, 0x53, 0x1b, 0x1f, 0x43, 0x17, 0x17, 0x33, 0x0f, 0x13, 0x23, 0x0b, 0x0b, 0x17, 0x07, 
  0xbf, 0xa7, 0x8f, 0xb7, 0x9f, 0x87, 0xaf, 0x97, 0x7f, 0xa7, 0x8f, 0x77, 0x9f, 0x87, 0x6f, 0x9b, 
  0x7f, 0x6b, 0x93, 0x7b, 0x63, 0x8b, 0x73, 0x5b, 0x83, 0x6b, 0x57, 0x7b, 0x63, 0x4f, 0x77, 0x5f, 
  0x4b, 0x6f, 0x57, 0x43, 0x67, 0x53, 0x3f, 0x5f, 0x4b, 0x37, 0x57, 0x43, 0x33, 0x53, 0x3f, 0x2f, 
  0x9f, 0x83, 0x63, 0x8f, 0x77, 0x53, 0x83, 0x6b, 0x4b, 0x77, 0x5f, 0x3f, 0x67, 0x53, 0x33, 0x5b, 
  0x47, 0x2b, 0x4f, 0x3b, 0x23, 0x43, 0x33, 0x1b, 0x7b, 0x7f, 0x63, 0x6f, 0x73, 0x57, 0x67, 0x6b, 
  0x4f, 0x5b, 0x63, 0x47, 0x53, 0x57, 0x3b, 0x47, 0x4f, 0x33, 0x3f, 0x47, 0x2b, 0x37, 0x3f, 0x27, 
  0xff, 0xff, 0x73, 0xeb, 0xdb, 0x57, 0xd7, 0xbb, 0x43, 0xc3, 0x9b, 0x2f, 0xaf, 0x7b, 0x1f, 0x9b, 
  0x5b, 0x13, 0x87, 0x43, 0x07, 0x73, 0x2b, 0x00, 0xff, 0xff, 0xff, 0xff, 0xdb, 0xdb, 0xff, 0xbb, 
  0xbb, 0xff, 0x9b, 0x9b, 0xff, 0x7b, 0x7b, 0xff, 0x5f, 0x5f, 0xff, 0x3f, 0x3f, 0xff, 0x1f, 0x1f, 
  0xff, 0x00, 0x00, 0xef, 0x00, 0x00, 0xe3, 0x00, 0x00, 0xd7, 0x00, 0x00, 0xcb, 0x00, 0x00, 0xbf, 
  0x00, 0x00, 0xb3, 0x00, 0x00, 0xa7, 0x00, 0x00, 0x9b, 0x00, 0x00, 0x8b, 0x00, 0x00, 0x7f, 0x00, 
  0x00, 0x73, 0x00, 0x00, 0x67, 0x00, 0x00, 0x5b, 0x00, 0x00, 0x4f, 0x00, 0x00, 0x43, 0x00, 0x00, 
  0xe7, 0xe7, 0xff, 0xc7, 0xc7, 0xff, 0xab, 0xab, 0xff, 0x8f, 0x8f, 0xff, 0x73, 0x73, 0xff, 0x53, 
  0x53, 0xff, 0x37, 0x37, 0xff, 0x1b, 0x1b, 0xff, 0x00, 0x00, 0xff, 0x00, 0x00, 0xe3, 0x00, 0x00, 
  0xcb, 0x00, 0x00, 0xb3, 0x00, 0x00, 0x9b, 0x00, 0x00, 0x83, 0x00, 0x00, 0x6b, 0x00, 0x00, 0x53, 
  0xff, 0xff, 0xff, 0xff, 0xeb, 0xdb, 0xff, 0xd7, 0xbb, 0xff, 0xc7, 0x9b, 0xff, 0xb3, 0x7b, 0xff, 
  0xa3, 0x5b, 0xff, 0x8f, 0x3b, 0xff, 0x7f, 0x1b, 0xf3, 0x73, 0x17, 0xeb, 0x6f, 0x0f, 0xdf, 0x67, 
  0x0f, 0xd7, 0x5f, 0x0b, 0xcb, 0x57, 0x07, 0xc3, 0x4f, 0x00, 0xb7, 0x47, 0x00, 0xaf, 0x43, 0x00, 
  0xff, 0xff, 0xff, 0xff, 0xff, 0xd7, 0xff, 0xff, 0xb3, 0xff, 0xff, 0x8f, 0xff, 0xff, 0x6b, 0xff, 
  0xff, 0x47, 0xff, 0xff, 0x23, 0xff, 0xff, 0x00, 0xa7, 0x3f, 0x00, 0x9f, 0x37, 0x00, 0x93, 0x2f, 
  0x00, 0x87, 0x23, 0x00, 0x4f, 0x3b, 0x27, 0x43, 0x2f, 0x1b, 0x37, 0x23, 0x13, 0x2f, 0x1b, 0x0b, 
  0x00, 0x00, 0x53, 0x00, 0x00, 0x47, 0x00, 0x00, 0x3b, 0x00, 0x00, 0x2f, 0x00, 0x00, 0x23, 0x00, 
  0x00, 0x17, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0xff, 0x9f, 0x43, 0xff, 0xe7, 0x4b, 0xff, 0x7b, 
  0xff, 0xff, 0x00, 0xff, 0xcf, 0x00, 0xcf, 0x9f, 0x00, 0x9b, 0x6f, 0x00, 0x6b, 0xa7, 0x6b, 0x6b
};
