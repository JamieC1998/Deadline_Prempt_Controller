//
// Created by jamiec on 9/27/22.
//

#ifndef CONTROLLER_TILEREGION_H
#define CONTROLLER_TILEREGION_H

namespace model {

    class TileRegion {
    public:
        TileRegion(int x1, int y1, int x2, int y2, int channelNumber);

        TileRegion();

        int getX1() const;

        int getY1() const;

        int getX2() const;

        int getY2() const;

        int getWidth() const;

        int getHeight() const;

        int getChannelNumber() const;

    private:
        int x1;
        int y1;
        int x2;
        int y2;
        int width;
        int height;
        int channel_number;
    };

} // model

#endif //CONTROLLER_TILEREGION_H
