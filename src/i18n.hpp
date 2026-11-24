#pragma once
#include <QString>

namespace gm {
using TranslationCallback = const char *(*)(const char *);
void setTranslationCallback(TranslationCallback callback);
QString ui(const char *key,const char *fallback);
}
