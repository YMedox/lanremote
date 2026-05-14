/*--------------------------------------------------------------------------------------------------------------------------------------
Парсинг простых XML-файлов. Возвращает всегда текстовое значение, которое нужно дальше преобразовать.
Код написан Гигакодом просто, чтобы не подключать никакие сторонние библиотеки, которые будут явно тяжелее, чем
набор одна функция getValue.
Исправлена 1 ошибка в реализации парсинга, и добавлена рекурсия для извлечения вложенных элементов.

Годится только для чтения значений 1 раз. Иначе, если искомый элемент вложенный - будет ненужный повторный парсинг.

Автор - Ярослав Медокс
--------------------------------------------------------------------------------------------------------------------------------------*/
#include "xml_parser.hpp"
#include <string>

bool cXmlParser::parse(const std::string& xmlContent) {
    // Очистка предыдущих данных
    elements_.clear();
    
    size_t pos = 0;
    while (pos < xmlContent.length()) {
        // Поиск открывающего тега
        size_t openTagStart = xmlContent.find('<', pos);
        if (openTagStart == std::string::npos) {
            break; // Больше тегов не найдено
        }
        
        // Пропускаем теги с атрибутами вроде <YAMAHA_AV rsp="GET" RC="0">
        if (openTagStart + 1 < xmlContent.length() && xmlContent[openTagStart + 1] != '/') {
            size_t openTagEnd = xmlContent.find('>', openTagStart);
            if (openTagEnd == std::string::npos) {
                break; // Некорректный XML
            }
            
            std::string openTag = xmlContent.substr(openTagStart, openTagEnd - openTagStart + 1);
            std::string tagName = getTagName(openTag);
            
            // Пропускаем закрывающие теги
            if (!isEndTag(openTag)) {
                // Ищем закрывающий тег
                std::string closeTag = "</" + tagName + ">";
                size_t closeTagPos = xmlContent.find(closeTag, openTagEnd);
                if (closeTagPos != std::string::npos) {
                    // Извлекаем значение между тегами
                    size_t valueStart = openTagEnd + 1;
                    size_t valueEnd = closeTagPos;
                    
                    // Пропускаем вложенные теги и пробелы в значении
                    std::string value = xmlContent.substr(valueStart, valueEnd - valueStart);
                    // Удаляем возможные пробелы в начале и конце
                    value.erase(0, value.find_first_not_of(" \t\r\n"));
                    value.erase(value.find_last_not_of(" \t\r\n") + 1);
                    
                    // Сохраняем элемент
                    elements_[tagName] = value;
                    
                    // Перемещаемся за закрывающий тег
//                    pos = closeTagPos + closeTag.length();  //Это ошибка, которую сделал Гигакод
                    pos = openTagEnd + 1;  

                } else {
                    // Самозакрывающийся тег или нет закрывающего тега
                    elements_[tagName] = "";
                    pos = openTagEnd + 1;
                }
            } else {
                pos = openTagEnd + 1;
            }
        } else {
            pos = openTagStart + 1;
        }
    }
    
    return true;
}

// Получение элемента. Добавил рекурсию на случай, если элемент вложен глубже.
// Алгоритм не эффективный, годится только для одноразового чтения. Но, в нашем случае чтение как раз одноразовое.
std::string cXmlParser::getValue(const std::string& elementName) const {
    auto it = elements_.find(elementName);
    if (it != elements_.end()) {
        return it->second;
    } else {
        size_t pos = 0;
        std::string tmp = elementName;
        std::string tmpName = elementName;
        pos = tmp.find('/');
        while(pos != std::string::npos) {
            tmpName = tmpName.substr(0, pos);
            cXmlParser nested;
            if(nested.parse(getValue(tmpName))) {
                tmp = tmp.substr(pos+1);
                pos = tmp.find('/', pos+1);
                if(pos == std::string::npos) { 
                    return nested.getValue(tmp); 
                }
            }
        }
    }
    return "";
}

bool cXmlParser::hasElement(const std::string& elementName) const {
    return elements_.find(elementName) != elements_.end();
}

std::string cXmlParser::extractElementName(const std::string& tag) const {
    std::size_t openBracket = tag.find('<');
    std::size_t closeBracket = tag.find('>');
    
    if (openBracket == std::string::npos || closeBracket == std::string::npos) {
        return "";
    }
    
    std::size_t start = openBracket + 1;
    std::size_t end = closeBracket;
    
    // Пропускаем '/' в начале для закрывающих тегов
    if (tag[start] == '/') {
        start++;
    }
    
    // Найти первый пробел или символ '/' (для самозакрывающихся тегов)
    std::size_t spacePos = tag.find(' ', start);
    if (spacePos != std::string::npos && spacePos < end) {
        end = spacePos;
    }
    
    std::size_t slashPos = tag.find('/', start);
    if (slashPos != std::string::npos && slashPos < end) {
        end = slashPos;
    }
    
    if (end <= start) {
        return "";
    }
    
    return tag.substr(start, end - start);
}

std::string cXmlParser::getTagName(const std::string& tag) const {
    if (tag.length() < 3 || tag[0] != '<') {
        return "";
    }
    
    return extractElementName(tag);
}

bool cXmlParser::isEndTag(const std::string& tag) const {
    return tag.length() > 3 && 
           tag.substr(0, 2) == "</" && 
           tag.back() == '>';
}