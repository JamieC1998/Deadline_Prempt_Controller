//
// Created by Jamie Cotter on 03/02/2023.
//

#ifndef CONTROLLER_FTP_CONFIG_H
#define CONTROLLER_FTP_CONFIG_H

namespace constant {
#define FTP_LOW_TIME 16861.2029
#define FTP_HIGH_TIME 11610.2628
#define FTP_LOW_N 1
#define FTP_LOW_M 2
#define FTP_LOW_CORE (FTP_LOW_M * FTP_LOW_N)
#define FTP_HIGH_N 2
#define FTP_HIGH_M 2
#define FTP_HIGH_CORE (FTP_HIGH_N * FTP_HIGH_M)
}

#endif //CONTROLLER_FTP_CONFIG_H