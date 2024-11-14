### Catalogs
There are currently 9 catalogs:


| File              | Description |
| ----------------- | ---------------------------------------- |
| ta_stars_vc       | 408 brightest stars                      |
| ta_stf_select_c   | Struve STF - 595 double stars            |
| ta_stt_select_c   | Struve STT - 114 double stars            |
| ta_gvcs_select_c  | 621 variable stars                       |
| ta_messier_c      | 109 Messier objects                      |
| ta_caldwell_c     | 109 Caldwell objects                     |
| ta_herschel_c     | 400 best NGC objects                     |
| ta_ngc_select_c   | ~ 2000 DSO objects of mag<16             |
| ta_ic_select_c    | ~ 500 DSO objects of mag<16              |


### Process for creating the catalogs

The original scripts (no longer available) worked as follows:

Databases --??????--> intermediate C files ---- Visual Basic scripts -----> C files included in TeenAstroCatalog



### New Scripts

The simplified python script (currently for NGC and IC objects only) has only one step:

OpenNGC database --- convertNGC.py -------> C files included in TeenAstroCatalog

It uses [the Python interface to ONGC](https://pyongc.readthedocs.io/en/latest/readme.html) to retrieve the data and the [Jinja templating engine](https://www.geeksforgeeks.org/getting-started-with-jinja-template/) to create the C files.


### Data Structures for NGC / IC objects

| Name          | Type           | Description                                                  |
| ------------- | -------------- | ------------------------------------------------------------ |
| Name          | unsigned short | not used                                                     |
| Constellation | unsigned char  | index into table of constellations                           |
| Object_type   | unsigned char  | index into table of object types                             |
| subId         | unsigned short | index into table of subId (optional letter after NGC / IC number) |
| Obj_id        | unsigned short | NGC / IC number                                              |
| Mag           | unsigned char  | magnitude, coded as (mag+2,5) * 10                           |
| RA            | unsigned short | RA, coded as RA(hours) * (65536 / 24)                        |
| DE            | unsigned short | Dec, coded as Dec(degrees) * (65536 / 180)                   |
