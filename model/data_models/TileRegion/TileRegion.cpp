//
// Created by jamiec on 9/27/22.
//

#include "TileRegion.h"

namespace model {
    TileRegion::TileRegion(int x1, int y1, int x2, int y2, int channelNumber) : x1(x1), y1(y1), x2(x2), y2(y2),
                                                                                channel_number(channelNumber), width(x2 - x1), height(y1 - y2) {}

    int TileRegion::getX1() const {
        return x1;
    }

    int TileRegion::getY1() const {
        return y1;
    }

    int TileRegion::getX2() const {
        return x2;
    }

    int TileRegion::getY2() const {
        return y2;
    }

    int TileRegion::getWidth() const {
        return width;
    }

    int TileRegion::getHeight() const {
        return height;
    }

    int TileRegion::getChannelNumber() const {
        return channel_number;
    }

    TileRegion::TileRegion() {}
} // model