PROJECT(DTIFiberTractStat)

SET(CXX_FILES
dtitractstat.cxx
fiberprocessing.cxx
regression.cxx
)

SET(H_FILES
dtitypes.h
argio.h
fiberprocessing.h
regression.h
)

SEMMacroBuildCLI(
    NAME dtitractstat
    ADDITIONAL_SRCS ${CXX_FILES} ${H_FILES}
    TARGET_LIBRARIES ${ITK_LIBRARIES} ${VTK_LIBRARIES}
    ${STATIC}
    INCLUDE_DIRECTORIES ${DTIFiberTractStat_SOURCE_DIR}
    INSTALL_ARCHIVE_DESTINATION ${ARCHIVE_DESTINATION}
    INSTALL_RUNTIME_DESTINATION ${RUNTIME_DESTINATION}
    INSTALL_LIBRARY_DESTINATION ${LIBRARY_DESTINATION}
    )

