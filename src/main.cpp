#ifdef _WIN32
#  include <windows.h>
#endif
#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glut.h>
#endif
#include "game.h"

static void displayFunc()
{
    gameDraw();
}

// Fixed 16 ms timestep (~60 FPS). Re-registers itself to keep the loop going.
static void timerFunc(int /*value*/)
{
    const float dt = 16.0f / 1000.0f;
    gameUpdate(dt);
    glutPostRedisplay();
    glutTimerFunc(16, timerFunc, 0);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Brick Breaker");

    glutDisplayFunc(displayFunc);
    glutTimerFunc(16, timerFunc, 0);
    glutKeyboardFunc(onKeyboard);
    glutSpecialFunc(onSpecial);
    glutSpecialUpFunc(onSpecialUp);
    glutMouseFunc(onMouse);
    glutPassiveMotionFunc(onPassiveMotion);
    glutReshapeFunc(onReshape);

    gameInit();
    onReshape(800, 600);   // set up projection before the first frame

    glutMainLoop();
    return 0;
}
