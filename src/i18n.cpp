#include "i18n.hpp"
#include <cstring>

namespace gm {
static TranslationCallback translate=nullptr;
void setTranslationCallback(TranslationCallback callback){translate=callback;}
QString ui(const char *key,const char *fallback)
{
    const char *value=translate?translate(key):nullptr;
    return QString::fromUtf8(value && std::strcmp(value,key)!=0?value:fallback);
}
}
