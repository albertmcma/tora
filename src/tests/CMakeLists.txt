

#FOREACH(item ${TORA_MOC_SOURCES})
#STRING(REGEX MATCH ".*/core/moc_.*c..$" item ${item})
#IF(item)
# LIST(APPEND CORE_OBJECT_SOURCES ${item})
#ENDIF(item)
#EgNDFOREACH(item ${TORA_MOC_SOURCES})

IF(TORA_DEBUG AND TEST_APP1)
# test1
ADD_EXECUTABLE("test1" ${GUI_TYPE}
  tests/test1.cpp
  ${PCH_SOURCE}  
  ${CORE_SOURCES}        #$<TARGET_OBJECTS:CORE_OBJECT_LIB>
  ${PARSING_SOURCES}     #$<TARGET_OBJECTS:PARSING_OBJECT_LIB>
  ${WIDGETS_SOURCES}     #$<TARGET_OBJECTS:WIDGETS_OBJECT_LIB>
  ${LOGGING_SOURCES}
  )
TARGET_LINK_LIBRARIES("test1"
	Qt5::Core
	Qt5::Widgets
	Qt5::Gui
	Qt5::Network
	${CMAKE_DL_LIBS}	
	${TORA_LOKI_LIB}
	${TORA_QSCINTILLA_LIB}
	${QSCINTILLA_LIBRARIES}
	antlr3c
)
SET_TARGET_PROPERTIES("test1" PROPERTIES ENABLE_EXPORTS ON)
IF(PCH_DEFINED)
  ADD_PRECOMPILED_HEADER("test1" ${PCH_HEADER} FORCEINCLUDE)
ENDIF(PCH_DEFINED)
ENDIF(TORA_DEBUG AND TEST_APP1)

IF(TORA_DEBUG AND TEST_APP2)  
# test2
ADD_EXECUTABLE("test2" ${GUI_TYPE}
  tests/test2.cpp
  ${PCH_SOURCE}  
  ${CORE_SOURCES}
  ${WIDGETS_SOURCES}
  ${LOGGING_SOURCES}
  )
TARGET_LINK_LIBRARIES("test2"
	Qt5::Core
	Qt5::Widgets
	Qt5::Gui
	Qt5::Network
	${CMAKE_DL_LIBS}
	${TORA_QSCINTILLA_LIB}
	${QSCINTILLA_LIBRARIES}
	${TORA_LOKI_LIB}
)
SET_TARGET_PROPERTIES("test2" PROPERTIES ENABLE_EXPORTS ON)
IF(PCH_DEFINED)
  ADD_PRECOMPILED_HEADER("test2" ${PCH_HEADER} FORCEINCLUDE)
ENDIF(PCH_DEFINED)
ENDIF(TORA_DEBUG AND TEST_APP2)

IF(TORA_DEBUG AND TEST_APP3)
# test3
QT5_WRAP_CPP(TEST3_MOC_SOURCES tests/test3window.h
	connection/tooracleconfiguration.h
	result/toresulttabledata.h
	tools/tobarchart.h
	tools/tobrowserbasewidget.h
	tools/todescribe.h	
	tools/tolinechart.h
	tools/toparamget.h
	tools/toresultbar.h
	tools/toresultcols.h
	tools/toresultdatasingle.h	
	tools/toresultplan.h
	tools/toresultstats.h
	tools/toresulttableview.h
	tools/toresulttableviewedit.h
	tools/toresultview.h
	tools/toworksheet.h
	tools/toworksheeteditor.h
	)
ADD_EXECUTABLE("test3" ${GUI_TYPE}
  tests/test3.cpp
  tests/test3window.cpp
  connection/tooracleconfiguration.cpp
  result/toresulttabledata.cpp
  tools/tobarchart.cpp
  tools/tobrowserbasewidget.cpp  
  tools/todescribe.cpp
  tools/tolinechart.cpp
  tools/toparamget.cpp
  tools/toresultbar.cpp
  tools/toresultcols.cpp
  tools/toresultdatasingle.cpp  
  tools/toresultplan.cpp
  tools/toresultstats.cpp
  tools/toresulttableview.cpp
  tools/toresulttableviewedit.cpp  
  tools/toresultview.cpp
  tools/toworksheet.cpp
  tools/toworksheeteditor.cpp
  ${TEST3_MOC_SOURCES}
  ${PCH_SOURCE}  
  ${CORE_SOURCES}    
  ${PARSING_SOURCES} 
  ${WIDGETS_SOURCES}
  ${EDITOR_SOURCES}
  ${LOGGING_SOURCES}
  )
TARGET_LINK_LIBRARIES("test3"
	Qt5::Core
	Qt5::Widgets
	Qt5::Gui
	Qt5::Network
	Qt5::PrintSupport
	${CMAKE_DL_LIBS}	
	${TORA_QSCINTILLA_LIB}
	${QSCINTILLA_LIBRARIES}	
	${TORA_LOKI_LIB}
	antlr3c	
	)
IF(PCH_DEFINED)
  ADD_PRECOMPILED_HEADER("test3" ${PCH_HEADER} FORCEINCLUDE)
ENDIF(PCH_DEFINED)
SET_TARGET_PROPERTIES("test3" PROPERTIES ENABLE_EXPORTS ON)
ENDIF(TORA_DEBUG AND TEST_APP3)

IF(TORA_DEBUG AND TEST_APP4)
# test4
QT5_WRAP_CPP(TEST4_MOC_SOURCES
  tests/test4window.h
  editor/tobaseeditor.h
  editor/tohighlightededitor.h
  editor/tomarkededitor.h  
  editor/tomemoeditor.h
  editor/toscintilla.h
  editor/tosqltext.h
  editor/tosyntaxanalyzernl.h
  editor/tosyntaxanalyzermysql.h
  editor/tosyntaxanalyzeroracle.h
  editor/tosyntaxanalyzerpostgresql.h
  tools/toresultstats.h
  tools/toresultview.h
  )
QT5_WRAP_UI(TEST4_UI_SOURCES tests/test4ui.ui)
QT5_ADD_RESOURCES(TEST4_RC_SOURCES tests/test4.qrc)
ADD_EXECUTABLE("test4"
  tests/test4.cpp
  tests/test4window.cpp
  editor/tobaseeditor.cpp
  editor/tohighlightededitor.cpp
  editor/tomarkededitor.cpp
  editor/tomemoeditor.cpp
  editor/toscintilla.cpp
  editor/tosqltext.cpp
  editor/tosyntaxanalyzernl.cpp
  editor/tosyntaxanalyzermysql.cpp
  editor/tosyntaxanalyzeroracle.cpp
  editor/tosyntaxanalyzerpostgresql.cpp
  tools/toresultstats.cpp
  tools/toresultview.cpp
  ${TEST4_MOC_SOURCES}
  ${TEST4_UI_SOURCES}
  ${TEST4_RC_SOURCES}
  ${PCH_SOURCE}  
  ${CORE_SOURCES}    
  ${PARSING_SOURCES} 
  ${WIDGETS_SOURCES} 
  ${LOGGING_SOURCES}
)
TARGET_LINK_LIBRARIES("test4"
	Qt5::Core
	Qt5::Widgets
	Qt5::Gui
	Qt5::Network
	Qt5::PrintSupport
	${CMAKE_DL_LIBS}
	${TORA_QSCINTILLA_LIB}
	${QSCINTILLA_LIBRARIES}	
	${TORA_LOKI_LIB}
	antlr3c
)
IF(PCH_DEFINED)
  ADD_PRECOMPILED_HEADER("test4" ${PCH_HEADER} FORCEINCLUDE)
ENDIF(PCH_DEFINED)
SET_TARGET_PROPERTIES("test4" PROPERTIES ENABLE_EXPORTS ON)
ENDIF(TORA_DEBUG AND TEST_APP4)

IF(TORA_DEBUG AND TEST_APP5)
# test5
QT5_WRAP_CPP(TEST5_MOC_SOURCES
  tests/test5window.h
  editor/tomarkededitor.h
  result/toresulttabledata.h
  tools/tobrowserbasewidget.h
  tools/todescribe.h
  tools/toparamget.h
  tools/toplsqleditor.h
  tools/toplsqltext.h
  tools/toresultview.h
  tools/toresultcols.h
  tools/toresultdatasingle.h
  tools/toresulttableview.h
  tools/toresulttableviewedit.h
	)
ADD_EXECUTABLE("test5"
  tests/test5.cpp
  tests/test5window.cpp
  editor/tomarkededitor.cpp
  result/toresulttabledata.cpp
  tools/tobrowserbasewidget.cpp
  tools/todescribe.cpp
  tools/toparamget.cpp
  tools/toplsqleditor.cpp
  tools/toplsqltext.cpp
  tools/toresultcols.cpp
  tools/toresultview.cpp
  tools/toresultdatasingle.cpp
  tools/toresulttableview.cpp
  tools/toresulttableviewedit.cpp
  ${TEST5_MOC_SOURCES}
  ${PCH_SOURCE}  
  ${CORE_SOURCES}   
  ${PARSING_SOURCES}
  ${WIDGETS_SOURCES}
  ${LOGGING_SOURCES}
  ${EDITOR_SOURCES}
  )
TARGET_LINK_LIBRARIES("test5"
	Qt5::Core
	Qt5::Widgets
	Qt5::Gui
	Qt5::Network
	Qt5::PrintSupport
	${CMAKE_DL_LIBS}
	${TORA_LOKI_LIB}
	${TORA_QSCINTILLA_LIB}
	${QSCINTILLA_LIBRARIES}
	antlr3c
)
IF(PCH_DEFINED)
  ADD_PRECOMPILED_HEADER("test5" ${PCH_HEADER} FORCEINCLUDE)
ENDIF(PCH_DEFINED)
SET_TARGET_PROPERTIES("test5" PROPERTIES ENABLE_EXPORTS ON)
ENDIF(TORA_DEBUG AND TEST_APP5)

IF(TORA_DEBUG AND TEST_APP6)
# test6
ADD_EXECUTABLE("test6" ${GUI_TYPE}
  tests/test6.cpp
  ${PCH_SOURCE}
  )
TARGET_LINK_LIBRARIES("test6"
	ermodel
	Qt5::Core
	Qt5::Widgets
	Qt5::Gui
)
IF(PCH_DEFINED)
  ADD_PRECOMPILED_HEADER("test6" ${PCH_HEADER} FORCEINCLUDE)
ENDIF(PCH_DEFINED)
SET_TARGET_PROPERTIES("test6" PROPERTIES ENABLE_EXPORTS ON)
ENDIF(TORA_DEBUG AND TEST_APP6)

IF(TORA_DEBUG AND TEST_APP7)
# test7
ADD_EXECUTABLE("test7"
  tests/test7.cpp  
  ${PCH_SOURCE}
  ${CORE_SOURCES}
  ${WIDGETS_SOURCES}
  ${PARSING_SOURCES}
  ${LOGGING_SOURCES}  
  )
TARGET_LINK_LIBRARIES("test7"
	Qt5::Core
	Qt5::Widgets
	Qt5::Gui
	Qt5::Network
	${CMAKE_DL_LIBS}
	${TORA_LOKI_LIB}	
	${TORA_QSCINTILLA_LIB}
	${QSCINTILLA_LIBRARIES}
	antlr3c
)
IF(PCH_DEFINED)
  ADD_PRECOMPILED_HEADER("test7" ${PCH_HEADER} FORCEINCLUDE)
ENDIF(PCH_DEFINED)
SET_TARGET_PROPERTIES("test7" PROPERTIES ENABLE_EXPORTS ON)
ENDIF(TORA_DEBUG AND TEST_APP7)

