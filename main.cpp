/*--------------------------------------------------------------------------------------------------------------------------------------
Управление media-устройствами по локальной сети.
Поддерживает ресиверы Yamaha и телевизоры Samsung до 2016г выпуска (взаимодействие TCP через socket).
Программный код управления Yamaha взят отсюда - https://github.com/synox/yamaha_avr_api
Доступные команды для Yamaha можно найти здесь https://github.com/PSeitz/yamaha-nodejs 
Программный код управления Samsung - см.файл tv_control.cpp
Команды описаны в конфигурационном файле и могут быть расширены.
Программа может работать с несколькими одинаковыми устройствами. Главное, чтобы у них имена в конф-файле были разные.

Автор - Ярослав Медокс
--------------------------------------------------------------------------------------------------------------------------------------*/

#include <string>
#include <curl/curl.h>

#include "iniobject.hpp"
#include "lanremote.hpp"
#include "grreader.hpp"
#include "tv_control.hpp"
#include "xml_parser.hpp" 
#include "version.h"


cLogger *logger = nullptr;

#define BUFFER_SIZE (256 * 1024) /* 256kB */
typedef struct point point_type;
struct http_response {
    char data [BUFFER_SIZE];
    int pos = 0;
};


// если не определена ни одна из констант, выдаст ошибку компиляции.
std::string getVersion() {
  std::string ret = "Version " + std::string(_v_.version) + ". Built on \"" + std::string(_v_.machine) + "\" " + std::string(_v_.date);
#ifdef __DEBUG__
  std::string ret1 =  ". Debug version.";
#else
  #ifdef __RELEASE__
    std::string ret1 =  ". Release version.";
  #endif
#endif
  return ret + ret1;
}

// Класс-обертка для curl.
class Http {
private:
    CURL* curl;
    struct curl_slist* slist;
    std::string response;
    static size_t read_response( void *ptr, size_t size, size_t nmemb, void *stream) {
        struct http_response* result = (struct http_response*)stream;
        /* Will we overflow on this write? */
        if(result->pos + size * nmemb >= BUFFER_SIZE - 1) {
            fprintf(stderr, "curl error: too small buffer\n");
            return 0;
        }
        /* Copy curl's stream buffer into our own buffer */
        memcpy(result->data + result->pos, ptr, size * nmemb);
        /* Advance the position */
        result->pos += size * nmemb;
        return size * nmemb;
    }
public:
    Http() {
        curl = curl_easy_init();
        slist = curl_slist_append(NULL, "Content-Type: text/xml; charset=utf-8");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, slist);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT , 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, read_response);
    }
    ~Http() {
        curl_slist_free_all(slist);
        curl_easy_cleanup(curl);
    }
    bool request(std::string& url, std::string xmlString) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POST , 1);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, xmlString.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE  , xmlString.size());
        // debug proxy:
        //curl_easy_setopt(curl, CURLOPT_PROXY, "127.0.0.1:8888");
        // Create the write buffer
        struct http_response write_result;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &write_result);
        CURLcode res = curl_easy_perform(curl);
        /* null terminate the std::string */
        write_result.data[write_result.pos] = '\0';
        if(res != CURLE_OK) {
            response = std::string("curl_easy_perform() failed: ") +  curl_easy_strerror(res);
            return false;
        }
        long http_code = 0;
        curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &http_code);
        if (http_code == 200 && res != CURLE_ABORTED_BY_CALLBACK)  {
            //Succeeded
            response = std::string(write_result.data);
        }  else        {
            //Failed
            std::string err = std::string("curl_easy_perform() failed: http response code: ") +  std::to_string(http_code) + std::string(write_result.data);
            logcpperror<<err<<ENDL;
        }
        return true;
    }
    std::string& getResponse() { return response; }
};

enum device_type { YAMAHA_AVR, SAMSUNG_OLDTV };
static std::map<int, char*> all_types = {{YAMAHA_AVR, (char*)"YamahaAVR"}, {SAMSUNG_OLDTV, (char*)"SamsungOldTV"}};


void printResult(bool result) {
   std::cout<<(result?"Done":"Error")<<std::endl; 
}

// Универсальный класс для управляемых устройств
class cDeviceForControl : public cGroupReader {
    private:
        int type_;
        std::string url;
        void runYamahaAVR(char *todo) {
            Http http;
            if(http.request(url, std::string(todo))) {
                logcppdebug<<"Response: "<<http.getResponse()<<ENDL;
              try {  // Непрямая логика. Если нет ошибки преобразования, значит это был запрос типа GET
                cXmlParser parser;
                if (parser.parse(http.getResponse())) {
                    std::vector<std::string> controls = {"Volume", "Power", "Sleep", "Mute", "Input_Sel", "Title", "Mode", "Speaker_A"};
                    int volume = std::stod(parser.getValue(controls[0]+"/Lvl/Val"))/10;
                    std::cout<<"{";
                    for(size_t i=0;i<controls.size();i++)  {
                        if(i) {
                            std::cout<<", ";
                            std::cout << "\"" << controls[i] << "\": \"" << parser.getValue(controls[i]) << "\""; 
                        } else {
                            std::cout << "\"" << controls[i] << "\": " << volume; 
                        }
                    }
                    std::cout<<"}"<<std::endl;
                } else {
                   printResult(false);
                }
              } catch (...) {
                printResult(true);    // а если была ошибка, значит это был запрос типа PUT, т.е. команда
              }
            } else { 
                printResult(false);
                logcpperror<<http.getResponse()<<ENDL;
            }
        }
        void runSamsungTV(char *todo) {
           SamsungTV tv(url);
           printResult(tv.sendKey(todo));  // Samsung не возвращает ответа для разбора
        }
    public:
      explicit cDeviceForControl(int type, char *ip) : type_(type) { 
        if(type_ == YAMAHA_AVR) {
           url = std::string("http://").append(ip).append("/YamahaRemoteControl/ctrl");
        } else {
           url = std::string(ip);
        }
      }
      // основной вызов выполнения команды
      void run(char *command) {
         char *todo = getValue(command);
         if(todo) {
            if(all_types.find(type_)!=all_types.end()) logcppinfo<<"Run "<<all_types[type_]<<" on "<<url<<" command: "<<todo<<ENDL;
            switch(type_) {
                case YAMAHA_AVR:
                    runYamahaAVR(todo);
                    break;
                case SAMSUNG_OLDTV:
                    runSamsungTV(todo);
                    break;
                default:
                    logcpperror<<"Unknown device type "<<type_<<ENDL;
                    break;
            }
         } else {
            printResult(false);
         }
      }
};
std::map<std::string, cDeviceForControl*> Devices;


//Запуск логирования
void initLogger(cIniObject *ini, bool bCout) {
   if(!logger) {
      cLogParams p;
      char *group  = (char*)"Logger";
      p.max_messages     = ini->getInt(group, (char*)"mmax", 100);
      p.max_size_of_file = ini->getInt(group, (char*)"fsize", 1000000);
      p.path             = ini->getString(group, (char*)"fpath");
      p.num_files        = ini->getInt(group, (char*)"fnum", 3);
      p.sink_cout        = bCout; // по умолчанию не выводим на экран
      p.sink_log         = true;
      p.tune_sleep       = true;  //этот логгер основной и имеет право регулировать скорость.
      p.log_level        = logger_getDebugLevelFromChar(ini->getString(group, (char*)"level"));
      logger = logger_new(&p);
      logger_setSleepTime(ini->getInt(group, (char*)"stime", 10000));
      if(!logger) {
        exit(1);
      }
      logcppinfo<<"Logger started with level "<<c_level[p.log_level]<<ENDL;
   }
}

//Чтение основных параметров программы
void readValues(char *config_file, bool bCout) {
  cIniObject ini(config_file);
  ini.setAllowSave(false);  // Запрещаем пересохранение конф-файла
  initLogger(&ini, bCout);
  cGroupReader devices;
  devices.fillAll(&ini, (char *) "Devices");
  char *device;
  for(gsize i=0; i<devices.getSize(); i++) {
    device = devices.getKey(i);
    if(device) {
        char *type = ini.getString(device, (char*)"type");
        char *ip   = ini.getString(device, (char*)"ip");
        if(strcmp(type, all_types[YAMAHA_AVR])==0) {
            Devices[device] = new cDeviceForControl(YAMAHA_AVR, ip);
            Devices[device]->fillAll(&ini, all_types[YAMAHA_AVR]);
        } else if(strcmp(type, all_types[SAMSUNG_OLDTV])==0) {
            Devices[device] = new cDeviceForControl(SAMSUNG_OLDTV, ip);
            Devices[device]->fillAll(&ini, all_types[SAMSUNG_OLDTV]);
        } else {
            logcpperror<<"Unknown device "<<device<<ENDL;
        }
    }
  }
}


void printUsage() {
  puts("Usage: lanremote [-v] [-l] [-c path_to_conf_file] Device Command  Device Command ...");    
}

// функция обработки аргументов программы передаваемых в командной строке
void manageArgv(int argc, char *argv[]) {
    int acount = 1;
    char *device;
    for(acount=1;acount<argc;acount++) {
      if(strncmp(argv[acount], "-", 1)!=0) {  //пошла команда
        device  = argv[acount++];
        if(Devices.find(device) != Devices.end()) {
            if(acount<argc) {
                char *command = argv[acount];
                logcppdebug<<"Arguments passed: Device: "<<device<<", command: "<<command<<ENDL;
                if(Devices[device]) {
                    Devices[device]->run(command);
                }
            }   else {
                logcpperror<<"No command passed for device: "<<device<<ENDL;
                printUsage();
            }
        } else {
            logcpperror<<"Unknown device: "<<device<<ENDL;
            printUsage();
        }
      }
    }
}


void cleanUp() {
    for(auto device : Devices)  {
        if(device.second) { delete device.second; }
    }
    Devices.clear();
    logger->flush();
    if(logger) delete logger;
}

int main( int argc, char **argv ) {
    char *config_file = nullptr;
    bool bCout = false;
    //сначала читаем только путь к конфигурационному файлу [-с path_to_file]
    //если параметры не указаны, то conf файл должен лежать рядом с исполняемым файлом
    int opt; // каждая следующая опция попадает сюда
    if(argc == 1) { printUsage(); exit(EXIT_SUCCESS); }
    while ((opt = getopt(argc, argv, "c:vl")) != -1) {
      switch (opt) {
        case 'c':   //путь к конф файлу. Если не задано, то ищет в том же каталоге, что и исполняемый файл
            config_file = optarg;
            break;
        case 'v':
            puts("lanremote. Control Yamaha receiver and Samsung TV");
            puts(getVersion().c_str());
            exit(EXIT_SUCCESS);
        case 'l':   //выводить сообщения на экран
            bCout = true;
            break;
        default:
            //puts("Usage: lanremote [-v] [-l] [-c path_to_conf_file] Device Command  Device Command ...");
            break;

      }
    }
    readValues(config_file, bCout);
    logcppinfo<<"lanremote started."<<ENDL;
    manageArgv(argc, argv);
    cleanUp();
    return EXIT_SUCCESS;
}