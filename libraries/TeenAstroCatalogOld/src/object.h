#pragma once

// Struct for Deep Space Objects (Messier, Herschel, ..etc.)
#pragma pack(1)

    typedef struct {
      const uint8_t        Cons:7;
      const uint8_t        Obj_type:7;
      const uint16_t       Obj_id;
      const uint8_t        Mag;
      const uint16_t       RA;
      const int16_t        DE;
    } dso_t;

 // Struct for stars
    typedef struct {
      const uint8_t        Cons:7;
      const uint8_t        Bayer;
      const uint8_t        Mag;
      const uint16_t       RA;
      const int16_t        DE;
    } star_t;
#pragma pack(0)