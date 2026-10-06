/*
===========================================================================
JKGunplay tight first/third-person camera
===========================================================================
*/

#ifndef CG_JKG_CAMERA_H
#define CG_JKG_CAMERA_H

qboolean JKG_TightCamActive( void );
void JKG_OffsetTightCamera( void );
qboolean JKG_ShowPlayerBody( void );
qboolean JKG_ViewWeaponReady( void );

void CG_OffsetFirstPersonView( qboolean firstPersonSaber );

#endif
