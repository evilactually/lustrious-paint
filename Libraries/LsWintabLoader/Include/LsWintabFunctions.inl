#if !defined(WT_EXPORTED_FUNCTION)
#define WT_EXPORTED_FUNCTION( fun )
#endif

WT_EXPORTED_FUNCTION( WTOpenA )
WT_EXPORTED_FUNCTION( WTInfoA )
WT_EXPORTED_FUNCTION( WTGetA )
WT_EXPORTED_FUNCTION( WTSetA )
WT_EXPORTED_FUNCTION( WTPacket )
WT_EXPORTED_FUNCTION( WTClose )
WT_EXPORTED_FUNCTION( WTEnable )
WT_EXPORTED_FUNCTION( WTOverlap )
WT_EXPORTED_FUNCTION( WTSave )
WT_EXPORTED_FUNCTION( WTConfig )
WT_EXPORTED_FUNCTION( WTRestore )
WT_EXPORTED_FUNCTION( WTExtSet )
WT_EXPORTED_FUNCTION( WTExtGet )
WT_EXPORTED_FUNCTION( WTQueueSizeSet )
WT_EXPORTED_FUNCTION( WTDataPeek )
WT_EXPORTED_FUNCTION( WTPacketsGet )

#undef WT_EXPORTED_FUNCTION