OTHER_FILES +=

SOURCES += \
    codeimpl.cpp \
    main.cpp \
    translatorvisitor.cpp \
    context.cpp

HEADERS += \
    translatorvisitor.hpp \
    codeimpl.hpp \
    ../../../../include/mathvm.h \
    ../../../../include/visitors.h \
    ../../../../include/ast.h \
    logger.hpp \
    context.hpp \
    translationerror.hpp \
    interpretationerror.hpp

INCLUDEPATH += ./../../../../include \
    ./../../../../vm
