#inlcude "BitmapParser.h"

using namespace DrawYourNoise;

BitmapParser::BitmapParser(char *data) :
    m_data(data)
{
}

BitmapParser::~BitmapParser()
{
    delete[] m_data;
}

std::shared_ptr<Image> BitmapParser::parse()
{
    auto image = std::make_shared<Image>();
    return image;
}