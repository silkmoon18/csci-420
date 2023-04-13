/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: Baihua Yang
 * *************************
 */

#include "utility.h"

vector<string> inputFiles;
Scene* scene;

void init() {
    glMatrixMode(GL_PROJECTION);
    glOrtho(0, WIDTH, 0, HEIGHT, 1, -1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
}

void display() {
}

void idle() {
    // hack to make it only draw once
    static int once = 0;
    if (!once) {
        for (int i = 0; i < inputFiles.size(); i++) {
            float start = Timer::getInstance()->getCurrentTime();

            scene->clear();
            scene->load(inputFiles[i].c_str());
            scene->render();

            float seconds = Timer::getInstance()->getCurrentTime() - start;
            printf("Time cost: %s\n", secondsToHMS(seconds).c_str());
        }
    }
    once = 1;
}

void readSceneType() {
    int type = 0;
    cout << "1. Phong\n2. Optical\n";
    while (true) {
        cout << "Choose a scene type: ";
        cin >> type;

        if (type == 1) {
            int level = 0;
            cout << "Enter the level of soft shadows (# light samples = 2 ^ softShadowLevel): ";
            cin >> level;
            scene = new PhongScene(level);
            break;
        }
        else if (type == 2) {
            int num = 0;
            cout << "Enter the number of sample rays per pixel: ";
            cin >> num;
            scene = new OpticalScene(num);
            break;
        }
        cout << endl
             << type << " is not valid.\n";
    }
    cout << endl;
}
void readFiles() {
    string path;
    while (true) {
        cout << "Enter the path to a scene file or a directory containing scene files: \n";
        cin >> path;

        filesystem::path input = path;
        if (!filesystem::exists(input)) {
            printf("%s does not exist. \n", path.c_str());
            continue;
        }

        if (filesystem::is_directory(input)) {
            for (const auto& entry : filesystem::directory_iterator(input)) {
                filesystem::path p = entry.path();

                string extension = p.extension().string();
                if (extension == ".scene") {
                    printf("Found scene: %s\n", p.filename().string().c_str());
                    inputFiles.push_back(p.string());
                }
            }
            int size = inputFiles.size();
            if (size > 0) {
                scene->mode = MODE_DISPLAY;
                printf("Found %d scene files in %s. \n", size, path.c_str());
                break;
            }
            printf("No scene files are found in %s. \n", path.c_str());
        }
        else if (input.extension().string() == ".scene") {
            inputFiles.push_back(input.string().c_str());

            break;
        }
        printf("%s is not a valid path. \n", path.c_str());
    }

    char ans;
    while (true) {
        cout << "Do you want to display the result? (Y/N) ";
        cin >> ans;
        ans = tolower(ans);
        if (ans == 'y') {
            scene->mode = MODE_DISPLAY;
            break;
        }
        else if (ans == 'n') {
            scene->mode = MODE_JPEG;
            break;
        }
        cout << ans << " is not valid.\n";
    }
    cout << endl;
}
void readAntiAliasingLevel() {
    int level = 0;
    cout << "Enter the level of anti-aliasing (# subpixels = 2 ^ antiAliasingLevel): ";
    cin >> level;
    scene->setAntiAliasingLevel(level);
}
void readThreadNumber() {
    int num = thread::hardware_concurrency();
    cout << "Using " << num << " threads for rendering. ";
    scene->setNumOfThreads(num);
}

int main(int argc, char** argv) {
    readSceneType();
    readFiles();
    readAntiAliasingLevel();
    readThreadNumber();

    cout << endl;

    glutInit(&argc, argv);

    glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(WIDTH, HEIGHT);
    int window = glutCreateWindow("Ray Tracer");
#ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
#endif
    glutDisplayFunc(display);
    glutIdleFunc(idle);
    init();
    glutMainLoop();
}
