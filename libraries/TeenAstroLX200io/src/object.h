#pragma once

// Struct for Deep Space Objects (Messier, Herschel, ..etc.)
    typedef struct {
      const uint8_t        Cons;
      const uint8_t        Obj_type;
      const uint16_t       Obj_id;
      const uint8_t        Mag;
      const uint16_t       RA;
      const int16_t        DE;
    } dso_t;

 // Struct for stars
    typedef struct {
      const uint8_t        Cons;
      const uint8_t        Bayer;
      const uint8_t        Mag;
      const uint16_t       RA;
      const int16_t        DE;
    } star_t;