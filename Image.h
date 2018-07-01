#ifndef IMAGE_H
#define IMAGE_H

namespace DrawYourNoise {
    class Image {
    public:
        using Dimension = Point;
        
        Pixel getPixel(const Point &point) const;
        void setPixel(const Point &point, const Pixel &pixel);
        
        int getHeight() const;
        int getWidth() const;
        Dimension getExtents() const;
        
        void setHeight(const int &height);
        void setWidth(const int &width);
        void setExtents(const Dimension &extents);
        
        Pixel& operator[](const Point &point);
        const Pixel& operator[](const Point &point) const;
        
    private:
        std::vector<Pixel> m_pixels;
        Dimension m_extents;
    };
}

#endif