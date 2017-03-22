#ifndef VERSIONNO__H
#define VERSIONNO__H


#define MAJOR_MULT             10000000
#define MINOR_MULT             100000

#define VERSION_FULL           0.0.183.0

#define VERSION_BASEYEAR       0
#define VERSION_DATE           "2017-03-22"
#define VERSION_TIME           "23:00:30"

#define VERSION_MAJOR          0
#define VERSION_MINOR          0
#define VERSION_BUILDNO        183
#define VERSION_EXTEND         0

#define SET_FIRWARE(V,SV,BN)		(V*MAJOR_MULT + SV*MINOR_MULT + BN ) //
#define OS_VERSION                	SET_FIRWARE(VERSION_MAJOR,VERSION_MINOR,VERSION_BUILDNO)

#define GET_MAJOR(A)             	((A)/MAJOR_MULT)
#define GET_MINOR(A)           		((A)-GET_MAJOR(A)*MAJOR_MULT)/MINOR_MULT
#define GET_BUILD(A)                (A)-GET_MAJOR(A)*MAJOR_MULT-GET_MINOR(A)*MINOR_MULT

#endif
