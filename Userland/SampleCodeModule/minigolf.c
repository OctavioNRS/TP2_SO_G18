#include <minigolf.h>
#include <stdint.h>
#include <syscalls.h>
#include <cstandard.h>
#include <hexColors.h>
#include <point.h>
#include <intMath.h>

//Keycodes:
#define KEY_W 0x11
#define KEY_A 0x1E
#define KEY_S 0x1F
#define KEY_D 0x20
#define KEY_UP 0xC7
#define KEY_LEFT 0xCA
#define KEY_RIGHT 0xCC
#define KEY_DOWN 0xCF
#define KEY_ESC 0x01
#define KEY_1 0x02
#define KEY_2 0x03
#define KEY_SPACE 0x39

#define GAME_SCREEN_WIDTH 1024
#define GAME_SCREEN_HEIGHT 768
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 16

#define SHADOW_COLOR 0x3F000000
#define SHADOW_OFFSET 3

#define PIT_COLOR 0x30000000
#define HILL_COLOR 0x30FFFFFF
#define PIT_SHADOW_COLOR 0x0F000000
#define HILL_LIGHT_COLOR 0x0FFFFFFF

#define ELEVATION_HEIGHT 5
#define MIN_CIRCLE_RADIUS 120
#define MAX_CIRCLE_RADIUS 180

#define MAX_CIRCLES 8
#define PLAYER_RADIUS 25
#define PLAYER_ACCEL 2.25
#define PLAYER_ROTATION_SPEED 540
#define SPEED_CAP 675

#define BALL_RADIUS 15
#define PUSH_FORCE 4
#define CIRCLE_FORCE_STRENGTH 0.2
#define BALL_IN_HOLE_MS 250
#define PLAYER_IN_HOLE_MS 1000
#define BOUNCE_SPEED_MULT 0.9

#define HOLD_ESC_MS 1000

static uint32_t bgColor = 0x007F00;
static double dragMultiplier = 1.0;

static uint8_t staticFrameBuffer[3*GAME_SCREEN_WIDTH*GAME_SCREEN_HEIGHT] = {0};
static uint8_t dynamicFrameBuffer[3*GAME_SCREEN_WIDTH*GAME_SCREEN_HEIGHT] = {0};
static int playing = 0;
static int deltaTime = 0;
static int globalTimer = 0;

static int holdEsc = 0;

typedef struct {
    int active;
    point2D position;
    point2D velocity;
    point2D lookDirection;
    int64_t radius;
    uint32_t color1;
    uint32_t color2;
    int64_t height;
    double drag;
    uint64_t speedCap;
    uint64_t inHoleMs;
    char *name;
} GameObject;

enum GameState {
    LEVEL,
    GENERATING,
    WIN,
    LOSE,
    LEVEL_ENDED,
    TITLE_SCREEN
};

static int state = TITLE_SCREEN;

static GameObject circles[MAX_CIRCLES];
static GameObject player1;
static GameObject player2;
static GameObject ball;
static GameObject hole;

static GameObject *possibleWinner;

static point2D screenCenter() {
    return (point2D){GAME_SCREEN_WIDTH/2,GAME_SCREEN_HEIGHT/2};
}

static point2D randomPosition() {
    int x = randomRange(0, GAME_SCREEN_WIDTH);
    int y = randomRange(0, GAME_SCREEN_HEIGHT);
    return point(x,y);
}

static void clampInsideScreen(GameObject *obj, int padding) {
    point2D pos = obj->position;
    if (pos.x - obj->radius < 0) pos.x = obj->radius+padding;
    if (pos.y - obj->radius < 0) pos.y = obj->radius+padding;
    if (pos.x + obj->radius > GAME_SCREEN_WIDTH) pos.x = GAME_SCREEN_WIDTH - obj->radius - padding;
    if (pos.y + obj->radius > GAME_SCREEN_HEIGHT) pos.y = GAME_SCREEN_HEIGHT - obj->radius - padding;
    obj->position = pos;
}

static int tryBounceOffScreen(GameObject *obj, point2D targetPos) {
    if ((targetPos.x - obj->radius) < 0 || targetPos.x + obj->radius > GAME_SCREEN_WIDTH) {
        obj->velocity.x *= -BOUNCE_SPEED_MULT;
        obj->velocity.y *= BOUNCE_SPEED_MULT;
        return -1;
    }
    else if ((targetPos.y - obj->radius) < 0 || targetPos.y + obj->radius > GAME_SCREEN_HEIGHT) {
        obj->velocity.y *= -BOUNCE_SPEED_MULT;
        obj->velocity.x *= BOUNCE_SPEED_MULT;
        return 1;
    }
    return 0;
}

static void handleOffscreen(GameObject *obj, point2D nextPos, int screenBorderPadding) {
    tryBounceOffScreen(obj, nextPos);
    obj->position = nextPos;
    clampInsideScreen(obj,1);
}

static void drawStringAround(char *str, uint64_t centerX, uint64_t y, uint32_t fgcolor, uint32_t bgcolor, int scale) {
    uint64_t textWidth = CHAR_WIDTH * strlen(str) * scale;

    uint64_t bottomLeftX = centerX - textWidth/2;

    sys_draw_string(str,bottomLeftX,y,fgcolor,bgcolor,scale);
}

static void drawElevation(GameObject circle) {
    sys_draw_circle(circle.color1,circle.position.x,circle.position.y,circle.radius,1);
    uint32_t shadeColor = HILL_LIGHT_COLOR;
    int shades = abs(circle.height);
    if (circle.height < 0) shadeColor = PIT_SHADOW_COLOR;
    int radiusStep = circle.radius/(shades+1);

    for (int i = 0; i < shades; i++)
    {
        int r = circle.radius-(i+1)*radiusStep;
        sys_draw_circle(shadeColor, circle.position.x, circle.position.y, r, 1);
    }
    
}

static GameObject randomCircle() {
    int r = randomRange(MIN_CIRCLE_RADIUS, MAX_CIRCLE_RADIUS);
    point2D pos = randomPosition();
    int pit = randomRange(0,1);
    int h = randomRange(ELEVATION_HEIGHT/2, ELEVATION_HEIGHT);
    uint32_t col = HILL_COLOR;
    if (pit) {
        h*=-1;
        col = PIT_COLOR;
    }
    return (GameObject){1,pos,ORIGIN,ORIGIN,r,col,0,h};
}

static int isOverlapping(GameObject obj1, GameObject obj2) {
    if (!obj1.active || !obj2.active) return 0;
    int squareDist = squareDistance(obj1.position,obj2.position);
    int radiusSum = obj1.radius+obj2.radius;
    return squareDist <= radiusSum*radiusSum;
}

static void generateCircles() {
    for (int i = 0; i < MAX_CIRCLES; i++)
    {
        circles[i].active = 0;
    }
    
    int circleCount = randomRange(MAX_CIRCLES*3/4,MAX_CIRCLES);

    for (int i = 0; i < circleCount; i++)
    {
        GameObject newCircle = randomCircle();
        int overlapFlag = 0;
        for (int rerolls = 0; rerolls < 1000; rerolls++)
        {
            overlapFlag = 0;
            if (isOverlapping(hole,newCircle)) {
                overlapFlag = 1;
            }
            else {
                for (int j = 0; j < i; j++)
                {
                    if (isOverlapping(circles[j],newCircle)) {
                        overlapFlag = 1;
                    }
                }
            }
            if (overlapFlag) {
                newCircle = randomCircle();
            }
            else break;
        }
        if (!overlapFlag) circles[i] = newCircle;
    }
}

static void drawCircles() {
    for (int i = 0; i < MAX_CIRCLES; i++)
    {
        if (circles[i].active) {
            drawElevation(circles[i]);
        }
    }
}

static void titleScreen() {
    sys_video_aux(staticFrameBuffer, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
    sys_background(bgColor);
    generateCircles();
    drawCircles();
    
    point2D center = screenCenter();
    point2D titleCenter = pointSub(center, point(0,200));
    sys_draw_rect(RED,titleCenter.x - 200,titleCenter.y - 80,400,100);
    drawStringAround("ARQ-GOLF",titleCenter.x,titleCenter.y,WHITE,-1,5);
    
    point2D selectionCenter = pointAdd(center, point(0,200));
    sys_draw_rect(BLACK,selectionCenter.x - 250,selectionCenter.y-40,500,140);
    drawStringAround("[1] Singleplayer",selectionCenter.x,selectionCenter.y,WHITE,-1,2);
    drawStringAround("[2] Versus",selectionCenter.x,selectionCenter.y+40,WHITE,-1,2);
    drawStringAround("[Hold Esc] Leave",selectionCenter.x,selectionCenter.y+80,WHITE,-1,2);

    sys_render_aux();
}

static GameObject newBall(point2D pos) {
    return (GameObject){1,pos,ORIGIN,ORIGIN,BALL_RADIUS,WHITE,LIGHT_GRAY,0,0.09*dragMultiplier,2*SPEED_CAP,0};
}

static GameObject newPlayer(uint32_t color, char *name) {
    return (GameObject){1,ORIGIN,ORIGIN,point(PLAYER_RADIUS,0),PLAYER_RADIUS,color,BLACK,0,0.12*dragMultiplier,SPEED_CAP,0,name};
}

static void waitForPlayerSelect() {
    int k1, k2;
    if ((k1 = sys_get_key(KEY_1)) || (k2 = sys_get_key(KEY_2))) {
        player1 = newPlayer(BLUE, "Player 1");
        player2 = k2 ? newPlayer(RED, "Player 2") : (GameObject){active: 0};
        state = GENERATING;
    }
}

static point2D randomWithin(int minX, int minY, int maxX, int maxY) {
    int x = randomRange(minX, maxX);
    int y = randomRange(minY, maxY);
    return point(x,y);
}

static void drawHole() {
    point2D center = hole.position;
    sys_draw_circle(BLACK, center.x, center.y, hole.radius, 1);
    sys_draw_rect(SHADOW_COLOR, center.x-2+SHADOW_OFFSET, center.y-150+SHADOW_OFFSET, 5, 150);
    sys_draw_rect(SHADOW_COLOR, center.x+3+SHADOW_OFFSET, center.y-150+SHADOW_OFFSET, 50, 30);
    sys_draw_rect(WHITE, center.x-2, center.y-150, 5, 150);
    sys_draw_rect(RED, center.x+3, center.y-150, 50, 30);
}

static void drawStaticObjects() {
    sys_video_aux(staticFrameBuffer, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
    sys_background(bgColor);
    drawCircles();
    drawHole();
}

static void drawLoadingScreen() {
    sys_video_aux(dynamicFrameBuffer, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
    sys_background(BLACK);
    point2D center = screenCenter();
    drawStringAround("Loading...", center.x, center.y, WHITE, -1, 3);
    sys_render_aux();
}

static void generateLevel() {
    drawLoadingScreen();
    point2D center = screenCenter();
    point2D startPos = randomWithin(0,0,center.x/2,center.y*2/3);
    int ballX = center.x/2;
    point2D holePos = randomWithin(center.x,150,GAME_SCREEN_WIDTH-50,GAME_SCREEN_HEIGHT-35);
    hole.radius = randomRange(26,35);
    point2D startLookDir = point(PLAYER_RADIUS,0);

    int flip = randomRange(0,1);
    if (flip) {
        startPos.x = GAME_SCREEN_WIDTH - startPos.x;
        ballX = GAME_SCREEN_WIDTH - ballX;
        holePos.x = GAME_SCREEN_WIDTH - holePos.x;
        startLookDir.x *= -1;
    }
    hole.position = holePos;
    hole.active = 1;
    hole.height = -ELEVATION_HEIGHT*1.5;
    generateCircles();
    point2D startPos2 = point(startPos.x, GAME_SCREEN_HEIGHT - startPos.y);
    
    player1.position = startPos;
    player1.lookDirection = startLookDir;
    player1.inHoleMs = 0;
    player2.position = startPos2;
    player2.lookDirection = startLookDir;
    player1.inHoleMs = 0;

    ball = newBall(point(ballX, center.y));

    drawStaticObjects();
    sys_play_sound(174,1);
    state = LEVEL;
}

static int fullyContained(GameObject target, GameObject area) {
    if (target.radius >= area.radius) return 0;
    int radiusDiff = area.radius - target.radius;
    return squareDistance(target.position,area.position) <= radiusDiff*radiusDiff;
}

static int closeToCenter(GameObject circle, GameObject obj) {
    double eps = 750 / circle.radius;
    return pointClose(circle.position, obj.position, eps);
}

static void applyCircleForce(GameObject *pullingCircle, GameObject *pulledObject) {
    // Calculo la distancia al cuadrado entre el centro del objeto y el centro del circulo
    point2D toCenter = pointSub(pullingCircle->position, pulledObject->position);
    uint64_t sqrDist = squareMagnitude(toCenter);
    
    if (closeToCenter(*pullingCircle, *pulledObject)) {
        return;
    }
    
    // Si el objeto esta fuera del circulo, no se aplica fuerza
    if (sqrDist > pullingCircle->radius*pullingCircle->radius) {
        return;
    }

    // Calculo la distancia exacta al centro
    uint64_t distance = magnitude(toCenter);
    
    // Calculo el multiplicador de fuerza (mas fuerte cerca del centro)
    double normalizedDistance = (double)distance / pullingCircle->radius;
    double forceMultiplier = -pullingCircle->height * (1.0 - normalizedDistance * normalizedDistance) * CIRCLE_FORCE_STRENGTH;

    point2D forceVector = pointScale(toCenter, forceMultiplier * deltaTime/(double)distance);
    
    // Aplico la fuerza a la velocidad (modifico en lugar)
    pulledObject->velocity = pointAdd(pulledObject->velocity, forceVector);

    return;
}

static void calculateFrameMovement(GameObject *obj) {
    if (!obj->active) return;

    for (int i = 0; i < MAX_CIRCLES; i++) {
        if (!circles[i].active) continue;
        applyCircleForce(&circles[i], obj);
    }
    applyCircleForce(&hole, obj);
    
    obj->velocity = pointScale(obj->velocity,1-obj->drag);
    if (squareMagnitude(obj->velocity) > obj->speedCap*obj->speedCap) {
        obj->velocity = setMagnitude(obj->velocity, obj->speedCap);
    }
    point2D move = pointScale(obj->velocity,deltaTime/(double)1000);
    point2D nextPos = pointAdd(obj->position,move);
    handleOffscreen(obj, nextPos, 1);
}

static void controlPlayer(GameObject *player, int forward, int left, int right, int back) {
    if (!player->active) return;
    int l = 0, r = 0, f = 0, b = 0;
    if ((l = sys_get_key(left))) {
        player->lookDirection = rotatePoint(player->lookDirection, -PLAYER_ROTATION_SPEED*deltaTime/(double)1000);
    }
    if ((r = sys_get_key(right))) {
        player->lookDirection = rotatePoint(player->lookDirection, PLAYER_ROTATION_SPEED*deltaTime/(double)1000);
    }
    point2D velocityChange = ORIGIN;
    if ((f = sys_get_key(forward))) {
        velocityChange = pointAdd(velocityChange, pointScale(player->lookDirection, (double)PLAYER_ACCEL));
    }
    if ((b = sys_get_key(back))) {
        velocityChange = pointAdd(velocityChange, pointScale(player->lookDirection, -(double)PLAYER_ACCEL/2));
    }
    if (l || r) {
        player->lookDirection = setMagnitude(player->lookDirection,PLAYER_RADIUS);
    }
    player->velocity = pointAdd(player->velocity,velocityChange);
    calculateFrameMovement(player);
}

static int tryPush(GameObject *source, GameObject *other) {
    if (!isOverlapping(*source, *other)) return 0;

    point2D delta = pointSub(other->position,source->position);
    point2D snapOffset = setMagnitude(delta, source->radius+other->radius+1);
    point2D snappedPos = pointAdd(source->position, snapOffset);
    other->position = snappedPos;

    int speed = magnitude(source->velocity);
    other->velocity = pointAdd(other->velocity, setMagnitude(delta,speed*PUSH_FORCE));

    return 1;
}

static void drawPlayer(GameObject player) {
    if (!player.active) return;
    sys_draw_circle(SHADOW_COLOR,player.position.x+5,player.position.y+5,player.radius,1);
    sys_draw_circle(player.color1,player.position.x,player.position.y,player.radius,1);
    point2D eye1 = rotatePoint(player.lookDirection,-20);
    point2D eye2 = rotatePoint(player.lookDirection,20);
    eye1 = pointAdd(player.position,pointScale(eye1,0.8));
    eye2 = pointAdd(player.position,pointScale(eye2,0.8));
    sys_draw_circle(player.color2,eye1.x,eye1.y,5,1);
    sys_draw_circle(player.color2,eye2.x,eye2.y,5,1);
}

static void drawBall(GameObject b) {
    if (!b.active) return;
    sys_draw_circle(SHADOW_COLOR,b.position.x+SHADOW_OFFSET,b.position.y+SHADOW_OFFSET,b.radius,1);
    sys_draw_circle(b.color2,b.position.x,b.position.y,b.radius,1);
    sys_draw_circle(b.color1,b.position.x,b.position.y,b.radius*0.9,1);
}

static void copyStaticToDynamic() {
    memcpy(dynamicFrameBuffer,staticFrameBuffer,3*GAME_SCREEN_WIDTH*GAME_SCREEN_HEIGHT);
}

static void drawGameplayFrame() {
    copyStaticToDynamic();
    sys_video_aux(dynamicFrameBuffer, GAME_SCREEN_WIDTH, GAME_SCREEN_HEIGHT);
    drawPlayer(player1);
    drawPlayer(player2);
    drawBall(ball);
    if (holdEsc > 0) {
        sys_draw_string("Keep holding esc to exit...", CHAR_WIDTH, CHAR_HEIGHT, WHITE, -1, 1);
        int progress = (holdEsc/(double)HOLD_ESC_MS)*80;
        sys_draw_rect(WHITE, 30*CHAR_WIDTH, 4, progress, CHAR_HEIGHT);
        sys_draw_rect(BLACK, 30*CHAR_WIDTH+progress, 4, 80-progress, CHAR_HEIGHT);
    }
    sys_render_aux();
}

static void drawActionSelection(point2D center) {
    sys_draw_rect(BLACK,center.x - 250,center.y,500,100);
    drawStringAround("[Space] Play again",center.x,center.y+40,WHITE,-1,2);
    drawStringAround("[Hold Esc] Leave",center.x,center.y+80,WHITE,-1,2);
}

static void drawMultiplayerEndScreen() {
    point2D center = screenCenter();
    point2D titleCenter = pointSub(center, point(0,200));
    sys_draw_rect(GREEN,titleCenter.x - 200,titleCenter.y - 100,400,150);
    drawStringAround(possibleWinner->name,titleCenter.x,titleCenter.y-40,possibleWinner->color1,-1,5);
    drawStringAround("WINS!",titleCenter.x,titleCenter.y+40,WHITE,-1,5);
    drawActionSelection(pointAdd(center, point(0,50)));
    
    sys_render_aux();
}

static void drawWinScreen() {
    point2D center = screenCenter();
    point2D titleCenter = pointSub(center, point(0,200));
    sys_draw_rect(GREEN,titleCenter.x - 200,titleCenter.y - 80,400,100);
    drawStringAround("YOU WIN!",titleCenter.x,titleCenter.y,WHITE,-1,5);
    drawActionSelection(pointAdd(center, point(0,50)));
    
    sys_render_aux();
}


static void drawLoseScreen() {
    point2D center = screenCenter();
    point2D titleCenter = pointSub(center, point(0,200));
    sys_draw_rect(RED,titleCenter.x - 200,titleCenter.y - 80,400,100);
    drawStringAround("YOU LOSE!",titleCenter.x,titleCenter.y,WHITE,-1,5);
    drawActionSelection(pointAdd(center, point(0,50)));
    
    sys_render_aux();
}

static int winCheck(GameObject *player) {
    if (fullyContained(*player, hole)) {
        player->inHoleMs+=deltaTime;
    }
    else player->inHoleMs = 0;
    if (closeToCenter(hole, ball)) {
        ball.inHoleMs+=deltaTime;
    }
    else ball.inHoleMs = 0;

    if (ball.inHoleMs >= BALL_IN_HOLE_MS) {
        state = WIN;
        return 1;
    }
    if (player->inHoleMs >= PLAYER_IN_HOLE_MS) {
        state = LOSE;
        return -1;
    }
    return 0;
}

static void playLevel() {

    controlPlayer(&player1, KEY_W, KEY_A, KEY_D, KEY_S);
    controlPlayer(&player2, KEY_UP, KEY_LEFT, KEY_RIGHT, KEY_DOWN);

    if (tryPush(&player1, &ball)) possibleWinner = &player1;
    if (tryPush(&player2, &ball)) possibleWinner = &player2;

    calculateFrameMovement(&ball);

    int p1State = winCheck(&player1);
    int p2State = winCheck(&player2);

    if (p1State == -1) {
        possibleWinner = &player2;
    }
    if (p2State == -1) {
        possibleWinner = &player1;
    }
    
    drawGameplayFrame();
}

static void waitForPlayAgain() {
    if (sys_get_key(KEY_SPACE)) {
        state = GENERATING;
    }
}

static void levelWon() {
    sys_play_sound(784,1);
    if (!player2.active) drawWinScreen();
    else drawMultiplayerEndScreen();
    state = LEVEL_ENDED;
}

static void levelLost() {
    sys_play_sound(110,1);
    if (!player2.active) drawLoseScreen();
    else drawMultiplayerEndScreen();
    state = LEVEL_ENDED;
}

static void update() {
    switch (state)
    {
    case LEVEL:
        playLevel();
        break;
    case LEVEL_ENDED:
        waitForPlayAgain();
        break;
    case GENERATING:
        generateLevel();
        break;
    case WIN:
        levelWon();
        break;
    case LOSE:
        levelLost();
        break;
    case TITLE_SCREEN:
        waitForPlayerSelect();
        break;
    }
    if (sys_get_key(KEY_ESC)) {
        holdEsc+=deltaTime;
        if (holdEsc >= HOLD_ESC_MS) {
            playing = 0;
        }
    }
    else holdEsc = 0;
}

void initializeMinigolf(int mode) {
    state = TITLE_SCREEN;

    switch (mode)
    {
    case 1:
        bgColor = 0x90D1DD;
        dragMultiplier = 0.33;
        break;
    
    default:
        bgColor = 0x007F00;
        dragMultiplier = 1.0;
        break;
    }

    playing = 1;
    globalTimer = 0;
    setRngSeed(sys_kernel_time());
    drawLoadingScreen();
    titleScreen();

    while(playing) {
        flushBuffer(STDIN);
        update();
        deltaTime = sys_sleep(1);
        globalTimer += deltaTime;
    }

    sys_video_aux(0,0,0);
}