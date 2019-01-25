#ifndef _ALTEKPARA_H_
#define _ALTEKPARA_H_

typedef struct {
    // Left camera
    double fx_L;
    double fy_L;
    double u0_L;
    double v0_L;

    double width_L;
    double height_L;

    // Right camera
    double fx_R;
    double fy_R;
    double u0_R;
    double v0_R;

    double width_R;
    double height_R;

    /*
    //LDC distortion rate at current VCM
    [0]:image height
    [1]:distortion rate
    */
    double distortion_curveL[2][22];
    double distortion_curveR[2][22];

    // 57-64
    double Rx;
    double Ry;
    double Rz;
    double Tx;
    double Ty;
    double Tz;

    int type1; // 0: Left_Right , 1: Up_Down
    int type2; // 0: Left or UP 1: Right or Down

} ALTEK_OUTPUT;

#endif
