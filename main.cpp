//使用方法：
//1. 按住鼠标左键拖动可以旋转图像
//2. 点击一个C和一个H（不分先后），显示键长
//3. 点击一个C和两个H（不分先后），显示键角
//4. 点击任意四个原子，显示二面角
//5. 点击黑色部分或点击多于四个原子，清空已经点击的记录，点击化学键不产生影响
//6. WSAD键可以缓慢旋转和移动图像
#include <windows.h>
#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>  //调用了求平方等数学方法

//宏定义，这些值后面会用到
#define SOLID 1
#define WIRE 2
#define RAND_MAX 0x7fff
 int ui_w = 500;
 int ui_h = 500;
int moveX, moveY;
int spinX = 0;
int spinY = 0;
int des = 0;
int click = 0;
int click_first = 0;
GLubyte nowColor[3];

//OpenGL的初始化函数
void init() {
	//设置背景色为黑色
	glClearColor(0.0, 0.0, 0.0, 0.0);
	//关闭光照，因为点击的时候是根据颜色判断这是什么原子的，如果打开光照，颜色会很复杂，就没办法取颜色了，所以都用了最简单的颜色
	glDisable(GL_LIGHTING);
}

//画圆柱体的函数，用来画化学键，六个参数分别是圆柱体的两端圆心坐标
//原理：要在A(x1,y1,z1), B(x2,y2,z2)之间绘制圆柱体，
//首先在原点处，沿着Y轴方向完成几何体绘制，然后旋转到AB向量方向，最后平移到A点处。
//关键在旋转矩阵的计算，使用向量叉乘：AB向量和Y轴单位向量叉乘计算出右手side向量，然后side单位化，side和AB叉乘计算出最终的up方向。
//这个函数算法很麻烦，你自己取舍要不要看懂……
void drawCylinder(float x0, float y0, float z0, float x1, float y1, float z1)
{
	//定义变量，如向量、长度等
	GLdouble  dir_x = x1 - x0;
	GLdouble  dir_y = y1 - y0;
	GLdouble  dir_z = z1 - z0;
	GLdouble  cylinderLength = sqrt(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);
	GLUquadricObj *quad_obj = gluNewQuadric();
	gluQuadricDrawStyle(quad_obj, GLU_FILL);
	gluQuadricNormals(quad_obj, GLU_SMOOTH);

	glPushMatrix();
	// 平移到起始点
	glTranslated(x0, y0, z0);
	// 计算长度
	double  length;
	length = sqrt(dir_x*dir_x + dir_y*dir_y + dir_z*dir_z);
	if (length < 0.0001) {
		dir_x = 0.0; dir_y = 0.0; dir_z = 1.0;  length = 1.0;
	}
	dir_x /= length;  dir_y /= length;  dir_z /= length;
	GLdouble  up_x, up_y, up_z;
	up_x = 0.0;
	up_y = 1.0;
	up_z = 0.0;
	double  side_x, side_y, side_z;
	side_x = up_y * dir_z - up_z * dir_y;
	side_y = up_z * dir_x - up_x * dir_z;
	side_z = up_x * dir_y - up_y * dir_x;
	length = sqrt(side_x*side_x + side_y*side_y + side_z*side_z);
	if (length < 0.0001) {
		side_x = 1.0; side_y = 0.0; side_z = 0.0;  length = 1.0;
	}
	side_x /= length;  side_y /= length;  side_z /= length;
	up_x = dir_y * side_z - dir_z * side_y;
	up_y = dir_z * side_x - dir_x * side_z;
	up_z = dir_x * side_y - dir_y * side_x;
	// 计算变换矩阵
	GLdouble  m[16] = {
		side_x, side_y, side_z, 0.0,
		up_x, up_y, up_z, 0.0,
		dir_x, dir_y, dir_z, 0.0,
		0.0, 0.0, 0.0, 1.0 };
	glMultMatrixd(m);
	// 圆柱体参数
	GLdouble radius = 0.25;        // 半径
	GLdouble slices = 32;      //  段数
	GLdouble stack = 5;       // 递归次数
	gluCylinder(quad_obj, radius, radius, cylinderLength, slices, stack);

	glPopMatrix();
}

//画球，用于画原子，4个是半径和球心坐标
void drawBall(double R, double x, double y, double z) {
	glPushMatrix();
		glTranslated(x, y, z);
		glutSolidSphere(R, 20, 20);
	glPopMatrix();
}

//显示字符的函数
void drawStr(const char* str) {
	static int isFirstCall = 1;
	static GLuint lists;

	if (isFirstCall) { // 如果是第一次调用，执行初始化
					   // 为每一个ASCII字符产生一个显示列表http://blog.csdn.net/shanzhizi
		isFirstCall = 0;

		// 申请MAX_CHAR个连续的显示列表编号
		lists = glGenLists(128);   //编号分别是lists, lists + 1, lists + 2, lists + MAX_CHAR -1

								   // 把每个字符的绘制命令都装到对应的显示列表中
		wglUseFontBitmaps(wglGetCurrentDC(), 0, 128, lists);  //从基数lists开始依次显示各个字符
	}
	glDeleteLists(lists, 1);
	// 调用每个字符对应的显示列表，绘制每个字符
	for (; *str != '\0'; ++str)
		glCallList(lists + *str);
}

//OpenGL的显示函数，用于绘图//
void display(void) {
	//清除缓冲区颜色和深度
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//开启深度测试，开启后可以实现图形的前后遮挡
	glEnable(GL_DEPTH_TEST);

	//根据点击情况显示对应的信息，调用了显示字符的函数
	//点击C和H，显示键长
	if (click == 1100)
	{//H-C
		glColor3f(0.9f, 0.9f, 0.9f);
		glRasterPos2f(3.0f, -8.0f);
		drawStr("109pm");
	}
	//点击两个H一个C，显示键角
	else if (click == 2100)
	{//H-C-H
		glColor3f(0.9f, 0.9f, 0.9f);
		glRasterPos2f(3.0f, -8.0f);
		drawStr("109 degs 28 mins");
	}
	//点击任意四个原子，显示二面角
	else if (click == 4000 || click == 3100) {
		glColor3f(0.9f, 0.9f, 0.9f);
		glRasterPos2f(3.0f, -8.0f);
		drawStr("70 degs 31 mins 43.6 secs ");
	}
	//点击超过四次，清空已经点击的
	else if (click > 4000) {
		click = 0;
	}

	//圆点放坐标中心
	glLoadIdentity();

	//摄像机位置，从哪个地方看
	gluLookAt(-2.0, -1.0, 20.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);

	glPushMatrix();
	glRotated(spinX, 0, 1, 0);
	glRotated(spinY, 1, 0, 0);
	glTranslated(0, 0, des);

	//绘制C原子，调用了画球的函数
	glColor4f(1.0, 0.0, 0.0, 1.0);//Red
	drawBall(2, 0, 0, 0);

	//绘制四个H原子，调用了画球的函数
	glColor4f(0.0, 0.0, 1.0, 1.0);//Blue
	drawBall(1, 0, 6.12, 0);
	drawBall(1, -5, -2.04, 2.89);
	drawBall(1, 5, -2.04, 2.89);
	drawBall(1, 0, -2.04, -5.77);

	//绘制化学键，调用了画圆柱体的函数
	glColor4f(0.0, 1.0, 0.0, 1.0);//green
	drawCylinder(0.0, 0.0, 0.0, 0.0, 6.12, 0.0);
	drawCylinder(0.0, 0.0, 0.0, -5, -2.04, 2.89);
	drawCylinder(0.0, 0.0, 0.0, 5, -2.04, 2.89);
	drawCylinder(0.0, 0.0, 0.0, 0, -2.04, -5.77);

	glPopMatrix();
	glutSwapBuffers();
}

//键盘事件（这个是附加功能，可以用键盘的wsad键旋转）
void keyPressed(unsigned char key, int x, int y) {

	switch (key) {
	case 'a':
		spinX -= 2;
		break;
	case 'd':
		spinX += 2;
		break;
	case 'w':
		des += 2;
		break;
	case 's':
		des -= 2;
		break;
	case ':':
		spinX += 1;
		glutPostRedisplay();
		spinX -= 1;
		break;
	}
	glutPostRedisplay();
}

//鼠标点击事件
void mouseClick(int btn, int state, int x, int y) {
	if (btn == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		if (click_first == 0) {
			glReadPixels(x, ui_h - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &nowColor);
			click_first++;
			keyPressed(':', 0, 0);
		}
		glutPostRedisplay();
		printf("x:%d,y:%d \n", x, 500 - y);

		glReadPixels(x, ui_h - y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, &nowColor);

		//根据颜色判断鼠标点到了什么上面， click变量用于储存点击产生的值，来判断点了几下H几下C
		//绿色 = 化学键 不改变click值
		if (nowColor[0] == 0 && nowColor[1] == 255 && nowColor[2] == 0) {
			printf("green\n");
			/*click += 1;*/
		}
		//红色 = C原子
		else if (nowColor[0] == 255 && nowColor[1] == 0 && nowColor[2] == 0) {
			click += 100;//每点一次C，click增加100
			printf("red\n");
		}
		//蓝色 = H原子
		else if (nowColor[0] == 0 && nowColor[1] == 0 && nowColor[2] == 255) {
			click += 1000;//每点一次H，click增加1000
			printf("blue\n");
		}
		//其他颜色 = 清空click
		else {
			click = 0;
		}
		printf("click = %d \n", click);
	}

}

// 鼠标移动事件，拖动以实现旋转
void mouseMove(int x, int y) {
	click = 0;
	int dx = x - moveX;
	int dy = y - moveY;
	spinX += dx;
	spinY += dy;
	glutPostRedisplay();
	moveX = x;
	moveY = y;
}

//OpenGL用于操作视窗大小的函数，可以让图像随着窗口大小的改变而改变
void reshape(int w, int h) {
	//定义视口大小
	glViewport(0, 0, (GLsizei)w, (GLsizei)h);
	ui_w = w;
	ui_h = h;

	//投影显示
	glMatrixMode(GL_PROJECTION);

	//坐标原点在屏幕中心
	glLoadIdentity();

	//操作模型视景
	gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 100.0);
	glMatrixMode(GL_MODELVIEW);
}


//Main函数，调用上面各个函数
int main(int argc, char** argv) {
	//初始化
	glutInit(&argc, argv);

	//设置显示模式
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	//初始化窗口大小
	glutInitWindowSize(ui_w, ui_h);

	//定义左上角窗口位置
	glutInitWindowPosition(100, 100);

	//创建窗口
	glutCreateWindow(argv[0]);

	//初始化
	init();

	//显示函数
	glutDisplayFunc(display);

	//窗口大小改变时的响应
	glutReshapeFunc(reshape);

	//鼠标点击事件，鼠标点击或者松开时调用
	glutMouseFunc(mouseClick);

	//鼠标移动事件，鼠标按下并移动时调用
	glutMotionFunc(mouseMove);

	//键盘事件
	glutKeyboardFunc(keyPressed);

	//循环
	glutMainLoop();

	return 0;
}
