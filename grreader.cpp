/*--------------------------------------------------------------------------------------------------------------------------------------
Вспомогательный класс для чтения групп ключей из конфигурационного файла.
По-хорошему надо переделать на stl-объекты

Автор - Ярослав Медокс
--------------------------------------------------------------------------------------------------------------------------------------*/
#include "grreader.hpp"
#include "mlogger.hpp"


// логгирование
extern cLogger *logger;

// класс для удобства чтения групп ключей из конфигурационного файла. 
cGroupReader::cGroupReader() { lines_ = 0; values_ = nullptr; }
cGroupReader::~cGroupReader() {
  logger->log(DEBUG, "%s%s%s", __FUNCTION__, "(). ", "Destroying instance of cGroupReader");
  for(gsize i=0;i<lines_;i++) {
    free(keys_[i]);
    if(values_) free(values_[i]);
  }
  if(lines_) {
    free(keys_);
    if(values_) free(values_);
  }
  lines_ = 0;
}
void cGroupReader::fillKeys(cIniObject *ini, const char *group) {
  if(lines_) {
    logger->log(ERROR, "%s%s%s", __FUNCTION__, "(). ", "fillKeys is allowed only once!");
    return;
  }
  keys_ = ini->getKeys(group, &lines_);
  if(!lines_) logger->log(WARN, "No keys in group %s", group);
}
gsize cGroupReader::getSize() { return int(lines_); }
void cGroupReader::fillValues(cIniObject *ini, const char *group) {
  if(lines_) {
    values_ = (char**)malloc(sizeof(char*)*lines_);
    for(gsize i=0;i<lines_;i++) {
      values_[i] = ini->getString(group, keys_[i], false);
      logger->log(DEBUG, "%s%s%s%s%s", __FUNCTION__, "(). ", keys_[i]," = ", (values_[i]?values_[i]:""));
    }
  } else {
    logger->log(ERROR, "%s%s%s%s%s", __FUNCTION__, "(). ", "No values or group ", group, " in conf file.");
  }
}
void cGroupReader::fillAll(cIniObject *ini, const char *group) {
  fillKeys(ini, group);
  fillValues(ini, group);
}
bool cGroupReader::checkInd(gsize ind) {
  if(/*ind >= 0 && */ind < lines_) { return true; }  // gsize всегда > 0
  else { logger->log(ERROR, "%s%s%s%d", __FUNCTION__, "(). ", "Wrong index passed: ", ind); }
  return false;
}
char *cGroupReader::getValue(gsize ind) {
  if(checkInd(ind)) if(values_) return values_[ind];
  return nullptr;
}
char *cGroupReader::getValue(char *key) {
   for(gsize i=0;i<lines_;i++) {
      if(!strcmp(keys_[i], key)) return values_[i];
   }
   logger->log(ERROR, "%s%s%s%s", __FUNCTION__, "(). ", "Couldn't find key ", key);
   return nullptr;
}
char *cGroupReader::getKey(gsize ind) {
  if(checkInd(ind)) return keys_[ind];
  return nullptr;
}
