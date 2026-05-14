#ifndef XML_PARSER_HPP
#define XML_PARSER_HPP

#include <string>
#include <map>

/**
 * Простой парсер XML для устройства Yamaha AV
 * Извлекает значения по именам элементов, игнорируя вложенность
 */
class cXmlParser {
public:
    cXmlParser() = default;
    
    /**
     * Парсит XML-строку
     * @param xmlContent Содержимое XML-файла в виде строки
     * @return true если парсинг успешен, false в случае ошибки
     */
    bool parse(const std::string& xmlContent);
    
    /**
     * Получает значение элемента по его имени
     * @param elementName Имя элемента (например, "Power", "Sleep", "Val" и т.д.)
     * @return Значение элемента или пустая строка если элемент не найден
     */
    std::string getValue(const std::string& elementName) const;
    
    /**
     * Проверяет наличие элемента
     * @param elementName Имя элемента
     * @return true если элемент найден, false если нет
     */
    bool hasElement(const std::string& elementName) const;

private:
    std::map<std::string, std::string> elements_; // Хранилище элементов
    
    // Вспомогательные методы
    std::string extractElementName(const std::string& tag) const;
    std::string getTagName(const std::string& tag) const;
    bool isEndTag(const std::string& tag) const;
};

#endif // YAMAHA_XML_PARSER_HPP