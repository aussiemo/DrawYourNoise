#ifndef BITMAP_PARSER_H
#define BITMAP_PARSER_H

namespace DrawYourNoise {
    class BitmapParser {
    public:
        BitmapParser(char *data);
        ~BitmapParser();
        
        std::shared_ptr<Image> parse() const;
    private:
        char *m_data;
    };
}

#endif