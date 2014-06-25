IF ( LLVM_FOUND )
    FUNCTION ( get_thorin_llvm_dependency_libs OUT_VAR )
    llvm_map_components_to_libraries ( THORIN_LLVM_TEMP_LIBS jit native analysis ipo )
    SET( THORIN_LLVM_TEMP_LIBS ${THORIN_LLVM_TEMP_LIBS} LLVMIRReader LLVMAsmParser LLVMBitReader LLVMBitWriter LLVMSupport )
    SET ( ${OUT_VAR} ${THORIN_LLVM_TEMP_LIBS} PARENT_SCOPE )
    ENDFUNCTION ()
ELSE ()
    FUNCTION ( get_thorin_llvm_dependency_libs OUT_VAR )
    SET ( ${OUT_VAR} PARENT_SCOPE )
    ENDFUNCTIOn ()
ENDIF ()

IF ( WFV2_FOUND )
    FUNCTION ( get_thorin_wfv2_dependency_libs OUT_VAR )
    get_wfv2_llvm_dependency_libs ( THORIN_WFV2_TEMP_LIBS )
    SET ( ${OUT_VAR} ${WFV2_LIBRARIES} ${THORIN_WFV2_TEMP_LIBS} PARENT_SCOPE )
    ENDFUNCTION ()
ELSE ()
    FUNCTION ( get_thorin_wfv2_dependency_libs OUT_VAR )
        SET ( ${OUT_VAR} PARENT_SCOPE )
    ENDFUNCTION ()
ENDIF ()

FUNCTION ( get_thorin_dependency_libs OUT_VAR )
    get_thorin_llvm_dependency_libs ( THORIN_LLVM_TEMP_LIBS )
    get_thorin_wfv2_dependency_libs ( THORIN_WFV2_TEMP_LIBS )
    SET ( ${OUT_VAR} ${THORIN_LLVM_TEMP_LIBS} ${THORIN_WFV2_TEMP_LIBS} PARENT_SCOPE )
ENDFUNCTION ()
