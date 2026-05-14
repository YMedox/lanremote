#ifndef GRREADER_HPP_INCLUDED
#define GRREADER_HPP_INCLUDED
#include "iniobject.hpp"


class cGroupReader {
  private:
  public:
      cGroupReader();
      ~cGroupReader();
      void fillKeys(cIniObject *ini, const char *group);
      void fillValues(cIniObject *ini, const char *group);
      void fillAll(cIniObject *ini, const char *group);
      gsize getSize();
      char *getValue(gsize ind);
      char *getValue(char *key);
      char *getKey(gsize ind);
  protected:
    bool checkInd(gsize ind);
    gsize lines_;
    char **keys_;
    char **values_;
};

#endif  // GRREADER_HPP_INCLUDED