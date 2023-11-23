/*
* "WORMS"
* 
* Ce programme reproduit de manière simplifiée la première version du jeu Worms,
* dans un style rétro, pixélisé.
* 
* Il est pour beaucoup basé sur le travail de One Loner Coder et utilise son
* moteur de jeu et graphisme personnel, "ConsoleGameEngine", visuellement très basique.
* 
* Le coeur de l'exercice repose sur l'étude de la physique MECANIQUE (mouvements d'objets
* soumis à la gravité, ballistique, collisions, rebonds), et à la mise en place d'une
* intelligence artificielle capable de STRATEGIE (offensive, défensive, neutre) choisie
* aléatoirement qui s'exprime ensuite par un comportement dépendant de la position des ennemis.
*
* 
* Il ne constitue pas tant un travail personnel qu'un exercice d'apprentissage du C++,
* des notions de mécanique dans un jeu et de la mise en place d'une IA agressive.
* 
* Eric Barzman
*/




#include "olcConsoleGameEngineGL.h"
#include <iostream>
#include <string>
#include <algorithm>
using namespace std;


// La classe dont vont hériter tous les objets, les armes et les worms
class cPhysicsObject
{
public:
    cPhysicsObject(float x = 0.0f, float y = 0.0f)
    {
        px = x;
        py = y;
    }

    float px = 0.0f;
    float py = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float ax = 0.0f;
    float ay = 0.0f;

    // Tous les objets sont considérés comme des sphères
    float radius = 4.0f;
    
    // A l'arrêt ?
    bool bStable = false;
    float fFriction = 0.8;

    // Combien de fois un objet rebondit-il avant de s'arrêter ?
    int nBounceBeforeDeath = -1;

    bool bDead = false;

    // La classe est abstraite
    virtual void Draw(olcConsoleGameEngine* engine, float fOffsetX, float fOffsetY, bool bPixel = false) = 0;
    virtual int BounceDeathAction() = 0;
    virtual bool Damage(float d) = 0;
};

// Un débris d'une explosion
class cDebris : public cPhysicsObject
{
public:
    cDebris(float x = 0.0f, float y = 0.0f) : cPhysicsObject(x, y)
    {
        vx = 10.0f * cosf(((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159);
        vy = 10.0f * sinf(((float)rand() / (float)RAND_MAX) * 2.0f * 3.14159);
        radius = 1.0f;
        fFriction = 0.8f;
        nBounceBeforeDeath = 5;
    }

    virtual void Draw(olcConsoleGameEngine* engine, float fOffsetX, float fOffsetY, bool bPixel = false)
    {
        engine->DrawWireFrameModel(vecModel, px - fOffsetX, py - fOffsetY, atan2f(vy, vx), bPixel ? 0.5f : radius, FG_DARK_GREEN);
    }

    // Ce qui se passe quand l'objet a fini ses rebonds
    virtual int BounceDeathAction()
    {
        return 0;
    }

    virtual bool Damage(float d)
    {
        return true; // Ne peut être endommagé, par défaut
    }

private:
    static vector<pair<float, float>> vecModel;
};


vector<pair<float, float>> DefineDebris()
{
    vector<pair<float, float>> vecModel;
    vecModel.push_back({ 0.0f, 0.0f });
    vecModel.push_back({ 1.0f, 0.0f });
    vecModel.push_back({ 1.0f, 1.0f });
    vecModel.push_back({ 0.0f, 1.0f });
    return vecModel;
}

vector<pair<float, float>> cDebris::vecModel = DefineDebris();

// Un missile
class cMissile : public cPhysicsObject
{
public:
    cMissile(float x = 0.0f, float y = 0.0f, float _vx = 0.0f, float _vy = 0.0f) : cPhysicsObject(x, y)
    {
        radius = 2.5f;
        fFriction = 0.5f;
        vx = _vx;
        vy = _vy;
        bDead = false;

        //Le missile explose dès contact, aucun rebond
        nBounceBeforeDeath = 1;
    }

    virtual void Draw(olcConsoleGameEngine* engine, float fOffsetX, float fOffsetY, bool bPixel = false)
    {
        engine->DrawWireFrameModel(vecModel, px - fOffsetX, py - fOffsetY, atan2f(vy, vx), bPixel ? 0.5f : radius, FG_BLACK);
    }

    virtual int BounceDeathAction()
    {
        return 20; // Grosse explosion
    }

    virtual bool Damage(float d)
    {
        return true; // Ne subit pas de dommages
    }

private:
    static vector<pair<float, float>> vecModel;
};

vector<pair<float, float>> DefineMissile()
{
    // la forme en fusée du missile
    vector<pair<float, float>> vecModel;
    vecModel.push_back({ 0.0f, 0.0f });
    vecModel.push_back({ 1.0f, 1.0f });
    vecModel.push_back({ 2.0f, 1.0f });
    vecModel.push_back({ 2.5f, 0.0f });
    vecModel.push_back({ 2.0f, -1.0f });
    vecModel.push_back({ 1.0f, -1.0f });
    vecModel.push_back({ 0.0f, 0.0f });
    vecModel.push_back({ -1.0f, -1.0f });
    vecModel.push_back({ -2.5f, -1.0f });
    vecModel.push_back({ -2.0f, 0.0f });
    vecModel.push_back({ -2.5f, 1.0f });
    vecModel.push_back({ -1.0f, 1.0f });

    // Mise à l'échelle unitaire
    for (auto& v : vecModel)
    {
        v.first /= 2.5f; v.second /= 2.5f;
    }
    return vecModel;
}
vector<pair<float, float>> cMissile::vecModel = DefineMissile();

// Nos petits soldats
class cWorm : public cPhysicsObject
{
public:
    cWorm(float x = 0.0f, float y = 0.0f) : cPhysicsObject(x, y)
    {
        radius = 3.5f;
        fFriction = 0.2f;
        bDead = false;

        //Ne rebondit pas
        nBounceBeforeDeath = -1;

        // Le sprite ne ressemble pas à un vers mais à un petit bonhomme
        if (sprWorm == nullptr)
            sprWorm = new olcSprite(L"./worms1.spr");
    }

    virtual void Draw(olcConsoleGameEngine* engine, float fOffsetX, float fOffsetY, bool bPixel = false)
    {
        if (bIsPlayable)
        {
            engine->DrawPartialSprite(px - fOffsetX - radius, py - fOffsetY - radius, sprWorm, nTeam * 8, 0, 8, 8);

            // Bar de santé
            for (int i = 0; i < 11 * fHealth; i++)
            {
                engine->Draw(px - 5 + i - fOffsetX, py - 9 - fOffsetY, PIXEL_SOLID, FG_BLUE);
                engine->Draw(px - 5 + i - fOffsetX, py - 8 - fOffsetY, PIXEL_SOLID, FG_BLUE);
            }
        }
        else  // Dessine une tombe
        {
            engine->DrawPartialSprite(px - fOffsetX - radius, py - fOffsetY - radius, sprWorm, nTeam * 8, 8, 8, 8);
        }
    }

    virtual int BounceDeathAction()
    {
        return 0;
    }

    // Calcul des dommages
    virtual bool Damage(float d)
    {
        fHealth -= d;
        if (fHealth <= 0)
        {
            fHealth = 0.0f;
            bIsPlayable = false;
        }
        return fHealth > 0;
    }

public:
    float fShootAngle = 0.0f;
    float fHealth = 1.0f;
    int nTeam = 0;	// ID de l'équipe du Worm
    bool bIsPlayable = true;

private:
    static olcSprite* sprWorm;
};

olcSprite* cWorm::sprWorm = nullptr;

// Les équipes
class cTeam
{
public:
    vector<cWorm*> vecMembers;
    int nCurrentMember = 0;
    int nTeamSize = 0;

    // L'équipe a-t-elle tjr des membres en vie ?
    bool IsTeamAlive()
    {
        bool bAllDead = false;
        for (auto w : vecMembers)
            bAllDead |= (w->fHealth > 0.0f);
        return bAllDead;
    }

    cWorm* GetNextNumber()
    {
        // Renvoie un pointer vers l'équipe suivante prête à être contrôlée
        do {
            nCurrentMember++;
            if (nCurrentMember >= nTeamSize) nCurrentMember = 0;
        } while (vecMembers[nCurrentMember]->fHealth <= 0);
        return vecMembers[nCurrentMember];
    }
};



// Le jeu, qui utilise Console Game Engine
class Worms : public olcConsoleGameEngine
{
public:
    Worms()
    {
        m_sAppName = L"Worms";
    }

private:

    //Ressources

    //Map
    int nMapWidth = 1024;
    int nMapHeight = 512;
    char* map = nullptr;

    //Camera
    float fCameraPosX = 0.0f;
    float fCameraPosY = 0.0f;
    float fCameraPosXTarget = 0.0f;
    float fCameraPosYTarget = 0.0f;

    // TOUS les objets du jeu
    list<unique_ptr<cPhysicsObject>> listObjects;

    cPhysicsObject* pObjectUnderControl = nullptr;  //pointer vers object contrôlé par joueur
    cPhysicsObject* pCameraTrackingObject = nullptr;//Point vers objet filmé par la caméra

    bool bZoomOut = false;
    bool bGameIsStable = false;
    bool bEnablePlayerControl = true;
    bool bEnableComputerControl = false;

    bool bEnergising = false;
    bool bFireWeapon = false;
    bool bShowCountDown = false;
    bool bPlayerHasFired = false;

    float fEnergyLevel = 0.0f;
    float fTurnTime = 0.0f;

    // Vector contenant les équipes
    vector<cTeam>vecTeams;

    int nCurrentTeam = 0;

    // Pour contrôler l'IA
    bool bAI_Jump = false;          // IA has appuyé sur jump
    bool bAI_AimLeft = false;       // IA vise à gauche etc.
    bool bAI_AimRight = false;
    bool bAI_Energise = false;      // Emmagasine la puissance de son tir

    float fAITargetAngle = 0.0f;
    float fAITargetEnergy = 0.0f;
    float fAISafePosition = 0.0f;
    cWorm* pAITargetWorm = nullptr;
    float fAITargetX = 0.0f;
    float fAITargetY = 0.0f;

    // Une énumération des game states
    enum GAME_STATE
    {
        GS_RESET = 0,
        GS_GENERATE_TERRAIN = 1,
        GS_GENERATING_TERRAIN,
        GS_ALLOCATE_UNITS,
        GS_ALLOCATING_UNITS,
        GS_START_PLAY,
        GS_CAMERA_MODE,
        GS_GAME_OVER1,
        GS_GAME_OVER2
    } nGameState, nNextState;

    // Les différentes phases du comportement de l'IA
    enum AI_STATE
    {
        AI_ASSESS_ENVIRONMENT = 0,
        AI_MOVE,
        AI_CHOOSE_TARGET,
        AI_POSITION_FOR_TARGET,
        AI_AIM,
        AI_FIRE
    } nAIState, nAINextState;

    // Création du jeu

    virtual bool OnUserCreate()
    {
        // Création map
        map = new char[nMapWidth * nMapHeight];
        memset(map, 0, nMapWidth * nMapHeight * sizeof(char));

        // Remet à zéro les states
        nGameState = GS_RESET;
        nNextState = GS_RESET;
        nAIState = AI_ASSESS_ENVIRONMENT;
        nAINextState = AI_ASSESS_ENVIRONMENT;

        bGameIsStable = false;
        return true;
    }



    /*
    * La boucle de jeu (Game Loop)
    * "On User Update"
    */
    virtual bool OnUserUpdate(float fElapsedTime)
    {
        // Tab permet de zoomer et dézoomer
        if (m_keys[VK_TAB].bReleased)
            bZoomOut = !bZoomOut;

        // Scroller la carte avec la souris
        float fMapScrollSpeed = 400.0f;
        if (m_mousePosX < 20) fCameraPosX -= fMapScrollSpeed * fElapsedTime;
        if (m_mousePosX > ScreenWidth() - 20) fCameraPosX += fMapScrollSpeed * fElapsedTime;
        if (m_mousePosY < 20) fCameraPosY -= fMapScrollSpeed * fElapsedTime;
        if (m_mousePosY > ScreenHeight() - 20) fCameraPosY += fMapScrollSpeed * fElapsedTime;


        // Définit les phases (states) du jeu
        switch (nGameState)
        {
        case GS_RESET:
        {
            bEnablePlayerControl = false;
            bGameIsStable = false;
            bPlayerHasFired = false;
            bShowCountDown = false;
            nNextState = GS_GENERATE_TERRAIN;
        }
        break;

        case GS_GENERATE_TERRAIN:
        {
            bZoomOut = false;
            CreateMap();
            bGameIsStable = false;
            bShowCountDown = false;
            nNextState = GS_GENERATING_TERRAIN;
        }
        break;

        case GS_GENERATING_TERRAIN:
        {
            bShowCountDown = false;
            if (bGameIsStable)
                nNextState = GS_ALLOCATE_UNITS;
        }
        break;

        case GS_ALLOCATE_UNITS:
        {
            // Déployer les équipes
            int nTeams = 4;
            int nWormsPerTeam = 3;

            // Calculer l'espacement
            float fSpacePerTeam = (float)nMapWidth / (float)nTeams;
            float fSpacePerWorm = fSpacePerTeam / (nWormsPerTeam * 2.0f);

            // Créer les équipes
            for (int t = 0; t < nTeams; t++)
            {
                vecTeams.emplace_back(cTeam());
                float fTeamMiddle = (fSpacePerTeam / 2.0f) + (t * fSpacePerTeam);
                for (int w = 0; w < nWormsPerTeam; w++)
                {
                    float fWormX = fTeamMiddle - ((fSpacePerWorm * (float)nWormsPerTeam) / 2.0f) + w * fSpacePerWorm;
                    float fWormY = 0.0f;

                    // Créer les Worms
                    cWorm* worm = new cWorm(fWormX, fWormY);
                    worm->nTeam = t;
                    listObjects.push_back(unique_ptr<cWorm>(worm));
                    vecTeams[t].vecMembers.push_back(worm);
                    vecTeams[t].nTeamSize = nWormsPerTeam;
                }

                vecTeams[t].nCurrentMember;
            }

            // Sélectionne le premier Worm a être joué et filmé
            pObjectUnderControl = vecTeams[0].vecMembers[vecTeams[0].nCurrentMember];
            pCameraTrackingObject = pObjectUnderControl;
            bShowCountDown = false;
            nNextState = GS_ALLOCATING_UNITS;
        }
        break;

        case GS_ALLOCATING_UNITS:
        {
            if (bGameIsStable)
            {
                bEnablePlayerControl = true;
                bEnableComputerControl = false;
                fTurnTime = 15.0f;
                bZoomOut = false;
                nNextState = GS_START_PLAY;
            }
        }
        break;
        // Le joueur contrôle le Worm et n'a pas encore tiré
        case GS_START_PLAY:
        {
            bShowCountDown = true;

            if (bPlayerHasFired || fTurnTime <= 0.0f)
                nNextState = GS_CAMERA_MODE;
        }
        break;
        // Missile tiré ! On suit l'action avec la caméra
        case GS_CAMERA_MODE:
        {
            bEnablePlayerControl = false;
            bEnableComputerControl = false;
            bPlayerHasFired = false;
            bShowCountDown = false;
            fEnergyLevel = 0.0f;

            // Une fois que tous les objets sont au repos
            if (bGameIsStable)
            {
                // Equipe suivante
                int nOldTeam = nCurrentTeam;
                do {
                    nCurrentTeam++;
                    nCurrentTeam %= vecTeams.size();
                } while (!vecTeams[nCurrentTeam].IsTeamAlive());

                // Bloque contrôle joueur quand l'IA joue
                if (nCurrentTeam == 0)
                {
                    bEnablePlayerControl = true;
                    bEnableComputerControl = false;
                }
                else
                {
                    bEnablePlayerControl = false;
                    bEnableComputerControl = true;
                }

                // Contrôles et camera
                pObjectUnderControl = vecTeams[nCurrentTeam].GetNextNumber();
                pCameraTrackingObject = pObjectUnderControl;
                fTurnTime = 15.0f;
                bZoomOut = false;
                nNextState = GS_START_PLAY;

                // S'il n'y a plus d'autres équipes...
                if (nCurrentTeam == nOldTeam)
                {
                    //... Game Over !
                    nNextState = GS_GAME_OVER1;
                }
            }
        }
        break;

        case GS_GAME_OVER1: // On dézoome et on célèbre ça par un grand tir de missiles !
        {
            bEnableComputerControl = false;
            bEnablePlayerControl = false;
            bZoomOut = true;
            bShowCountDown = false;

            for (int i = 0; i < 100; i++)
            {
                int nBombX = rand() % nMapWidth;
                int nBombY = rand() % (nMapHeight / 2);
                listObjects.push_back(unique_ptr<cMissile>(new cMissile(nBombX, nBombY, 0.0f, 0.5f)));
            }

            nNextState = GS_GAME_OVER2;
        }
        break;

        case GS_GAME_OVER2: // On attend le retour du calme
        {
            bEnableComputerControl = false;
            bEnablePlayerControl = false;
            // Il n'y a pas de sortie de cette phase, le jeu n'a pas de fin en soi
        }
        break;
        }

        if (bEnableComputerControl)
        {
            // PHASES DE L'IA
            switch (nAIState)
            {
            // Etudier l'environnement avant d'agir
            case AI_ASSESS_ENVIRONMENT:
            {
                // Choisit aléatoirement entre trois options
                int nAction = rand() % 3;
                if (nAction == 0)
                // On va la jouer défensif : le Worm s'éloigne de ses alliés
                // pour augmenter leur chance de survie
                {
                    // Prend son allié le plus proche
                    float fNearestAllyDistance = INFINITY; float fDirection = 0;
                    cWorm* origin = (cWorm*)pObjectUnderControl;

                    for (auto w : vecTeams[nCurrentTeam].vecMembers)
                    {
                        if (w != pObjectUnderControl)
                        {
                            if (fabs(w->px - origin->px) < fNearestAllyDistance)
                            {
                                fNearestAllyDistance = fabs(w->px - origin->px);
                                fDirection = (w->px - origin->px) < 0.0f ? 1.0f : -1.0f;
                            }
                        }
                    }

                    // Calcule où il doit aller se placer pour être à une distance safe
                    if (fNearestAllyDistance < 50.f)
                        fAISafePosition = origin->px + fDirection * 80.0f;
                    else
                        fAISafePosition = origin->px;
                }

                if (nAction == 1)
                // Offensif ! Le Worm va au milieu de l'écran, là d'où il peut toucher tout le monde
               
                {
                    cWorm* origin = (cWorm*)pObjectUnderControl;
                    float fDirection = ((float)(nMapWidth / 2.0f) - origin->px) < 0.0f ? -1.0f : 1.0f;
                    fAISafePosition = origin->px + fDirection * 200.0f;
                }

                if (nAction == 2)  // Fait le mort. Neutre. Ne bouge pas.
                {
                    cWorm* origin = (cWorm*)pObjectUnderControl;
                    fAISafePosition = origin->px;
                }

                // Empêche les Worms de quitter la map
                if (fAISafePosition <= 20.0f) fAISafePosition = 20.0f;
                if (fAISafePosition >= nMapWidth - 20.0f) fAISafePosition = nMapWidth - 20.0f;

                nAINextState = AI_MOVE;
            }
            break;

            // L'IA se met en mouvement
            case AI_MOVE:
            {
                cWorm* origin = (cWorm*)pObjectUnderControl;
                if (fTurnTime >= 8.0f && origin->px != fAISafePosition)
                {
                    // Sautille jusqu'à la position calculée précédemment
                    if (fAISafePosition < origin->px && bGameIsStable)
                    {
                        origin->fShootAngle = -3.14159f * 0.6f;
                        bAI_Jump = true;
                        nAINextState = AI_MOVE;
                    }
               
                    if (fAISafePosition > origin->px && bGameIsStable)
                    {
                        origin->fShootAngle = -3.14159 * 0.04f;
                        bAI_Jump = true;
                        nAINextState = AI_MOVE;
                    }
                }
                else
                    nAINextState = AI_CHOOSE_TARGET;
            }
            break;

            // Le Worm a fini de bouger, il choisit sa cible
            case AI_CHOOSE_TARGET:  
            {
                bAI_Jump = false;

                // Choisit une autre équipe que la sienne
                cWorm* origin = (cWorm*)pObjectUnderControl;
                int nCurrentTeam = origin->nTeam;
                int nTargetTeam = 0;
                do {
                    nTargetTeam = rand() % vecTeams.size();
                } while (nTargetTeam == nCurrentTeam || !vecTeams[nTargetTeam].IsTeamAlive());

                // Il va choisir le Worm adversaire avec la bar de santé la plus forte	
                cWorm* mostHealthyWorm = vecTeams[nTargetTeam].vecMembers[0];
                for (auto w : vecTeams[nTargetTeam].vecMembers)
                    if (w->fHealth > mostHealthyWorm->fHealth)
                        mostHealthyWorm = w;

                pAITargetWorm = mostHealthyWorm;
                fAITargetX = mostHealthyWorm->px;
                fAITargetY = mostHealthyWorm->py;
                nAINextState = AI_POSITION_FOR_TARGET;
            }
            break;

            // Calcule la position adaptée pour tirer sur sa cible.
            // S'il doit bouger de nouveau, il le fait
            case AI_POSITION_FOR_TARGET:
            {
                cWorm* origin = (cWorm*)pObjectUnderControl;
                float dy = -(fAITargetY - origin->py);
                float dx = -(fAITargetX - origin->px);
                float fSpeed = 30.0f;
                float fGravity = 2.0f;

                bAI_Jump = false;

                float a = fSpeed * fSpeed * fSpeed * fSpeed - fGravity * (fGravity * dx * dx + 2.0f * dy * fSpeed * fSpeed);

                if (a < 0)   // La cible est trop loin
                {
                    // Il y a encore le temps. Le Worm va bouger.
                    if (fTurnTime >= 5.0f)
                    {
                        if (pAITargetWorm->px < origin->px && bGameIsStable)
                        {
                            origin->fShootAngle = -3.14159f * 0.6f;
                            bAI_Jump = true;
                            nAINextState = AI_POSITION_FOR_TARGET;
                        }

                        if (pAITargetWorm->px > origin->px && bGameIsStable)
                        {
                            origin->fShootAngle = -3.14159f * 0.4f;
                            bAI_Jump = true;
                            nAINextState = AI_POSITION_FOR_TARGET;
                        }
                    }
                    else  // Il n'y a plus assez de temps. Le Worm va tirer malgré tout.
                    {
                        fAITargetAngle = origin->fShootAngle;
                        fAITargetEnergy = 0.75f;
                        nAINextState = AI_AIM;
                    }
                }
                else
                {
                    // Le Worm est assez près de sa cible. Calcul de la trajectoire
                    float b1 = fSpeed * fSpeed + sqrtf(a);
                    float b2 = fSpeed * fSpeed - sqrtf(a);

                    // Deux hauteurs permettent de toucher un point, en ballistique
                    float fTheta1 = atanf(b1 / (fGravity * dx));  //Hauteur la plus grande
                    float fTheta2 = atanf(b2 / (fGravity * dx));  //Hauteur la plus petite

                    // La hauteur max a plus de chance de faire passer
                    // le missile au-dessus des obstacles.
                    // Le Worm choisit celle-ci et calcule l'angle.
                    fAITargetAngle = fTheta1 - (dx > 0 ? 3.14159f : 0.0f);
                    float fFireX = cosf(fAITargetAngle);
                    float fFireY = sinf(fAITargetAngle);

                    fAITargetEnergy = 0.75f;
                    nAINextState = AI_AIM;
                }
            }
            break;

            case AI_AIM:  // Le Worm vise
            {
                cWorm* worm = (cWorm*)pObjectUnderControl;

                bAI_AimLeft = false;
                bAI_AimRight = false;
                bAI_Jump = false;

                if (worm->fShootAngle < fAITargetAngle)
                    bAI_AimRight = true;
                else
                    bAI_AimLeft = true;

                // Il a trouvé le bon angle
                if (fabs(worm->fShootAngle - fAITargetAngle) <= 0.001f)
                {
                    bAI_AimLeft = false;
                    bAI_AimRight = false;
                    fEnergyLevel = 0.0f;
                    nAINextState = AI_FIRE;
                }
                else
                    nAINextState = AI_AIM;

            }
            break;

            // L'IA tire
            case AI_FIRE:
            {
                bAI_Energise = true;
                bFireWeapon = false;
                bEnergising = true;

                if (fEnergyLevel >= fAITargetEnergy)
                {
                    bFireWeapon = true;
                    bAI_Energise = false;
                    bEnergising = false;
                    bEnableComputerControl = false;
                    nAINextState = AI_ASSESS_ENVIRONMENT;
                }
            }
            break;
            }
        }

        fTurnTime -= fElapsedTime;

        if (pObjectUnderControl != nullptr)
        {
            pObjectUnderControl->ax = 0.0f;

            if (pObjectUnderControl->bStable)
            {
                // Contrôle les déplacemnets joueur ET l'IA par la même occasion

                // Saute
                if ((bEnablePlayerControl && m_keys[L'Z'].bPressed) || (bEnableComputerControl && bAI_Jump))
                {
                    float a = ((cWorm*)pObjectUnderControl)->fShootAngle;

                    pObjectUnderControl->vx = 4.0f * cosf(a);
                    pObjectUnderControl->vy = 8.0f * sinf(a);
                    pObjectUnderControl->bStable = false;

                    bAI_Jump = false;
                }

                // Vise vers la gauche
                if ((bEnablePlayerControl && m_keys[L'Q'].bHeld) || (bEnableComputerControl && bAI_AimLeft))
                {
                    cWorm* worm = (cWorm*)pObjectUnderControl;
                    worm->fShootAngle -= 1.0f * fElapsedTime;
                    if (worm->fShootAngle < -3.14159f) worm->fShootAngle += 3.14159f * 2.0f;
                }

                // Vise vers la droite
                if ((bEnablePlayerControl && m_keys[L'D'].bHeld) || (bEnableComputerControl && bAI_AimRight))
                {
                    cWorm* worm = (cWorm*)pObjectUnderControl;
                    worm->fShootAngle += 1.0f * fElapsedTime;
                    if (worm->fShootAngle > 3.14159f) worm->fShootAngle -= 3.14159f * 2.0f;
                }

                // Espace : emmagasine puissance du tir
                if (bEnablePlayerControl && m_keys[VK_SPACE].bPressed)
                {
                    bEnergising = true;
                    fEnergyLevel = 0.0f;
                    bFireWeapon = false;
                }

                if ((bEnablePlayerControl && m_keys[VK_SPACE].bHeld) || (bEnableComputerControl && bAI_Energise))
                {
                    if (bEnergising)
                    {
                        fEnergyLevel += 0.75f * fElapsedTime;
                        if (fEnergyLevel >= 1.0f)
                        {
                            fEnergyLevel = 1.0f;
                            bFireWeapon = true;
                        }
                    }
                }
                // Tire !
                if (bEnablePlayerControl && m_keys[VK_SPACE].bReleased)
                {
                    if (bEnergising)
                    {
                        bFireWeapon = true;
                    }

                    bEnergising = false;
                }
            }

            // La caméra
            if (pCameraTrackingObject != nullptr)
            {
                fCameraPosXTarget = pCameraTrackingObject->px - ScreenWidth() / 2;
                fCameraPosYTarget = pCameraTrackingObject->py - ScreenHeight() / 2;
                fCameraPosX += (fCameraPosXTarget - fCameraPosX) * 5.0f * fElapsedTime;
                fCameraPosY += (fCameraPosYTarget - fCameraPosY) * 5.0f * fElapsedTime;
            }

            if (bFireWeapon)
            {
                cWorm* worm = (cWorm*)pObjectUnderControl;

                //Origine du tir
                float ox = worm->px;
                float oy = worm->py;

                //Direction du tir
                float dx = cosf(worm->fShootAngle);
                float dy = sinf(worm->fShootAngle);

                //Crée un missile
                cMissile* m = new cMissile(ox, oy, dx * 40.0f * fEnergyLevel, dy * 40.0f * fEnergyLevel);
                listObjects.push_back(unique_ptr<cMissile>(m));

                //Attache la caméra au missile
                pCameraTrackingObject = m;

                bFireWeapon = false;
                fEnergyLevel = 0.0f;
                bEnergising = false;

                bPlayerHasFired = true;

                if (rand() % 100 >= 50)
                    bZoomOut = true;
            }
        }


        // Bloque la caméra dans les limites de la map
        if (fCameraPosX < 0) fCameraPosX = 0;
        if (fCameraPosX >= nMapWidth - ScreenWidth()) fCameraPosX = nMapWidth - ScreenWidth();
        if (fCameraPosY < 0) fCameraPosY = 0;
        if (fCameraPosY >= nMapHeight - ScreenHeight()) fCameraPosY = nMapHeight - ScreenHeight();


        //On répète 10 itérations par frames (nécessaire pour le gameplay)
        for (int z = 0; z < 10; z++)
        {
            //Update les objets
            for (auto& p : listObjects)
            {
                // Applique la gravité
                p->ay += 2.0f;

                // L'accélération agit sur la vitesse
                p->vx += p->ax * fElapsedTime;
                p->vy += p->ay * fElapsedTime;

                // La vitesse agit sur la position des objets
                float fPotentialX = p->px + p->vx * fElapsedTime;
                float fPotentialY = p->py + p->vy * fElapsedTime;

                // Reset l'accélération
                p->ax = 0.0f;
                p->ay = 0.0f;
                p->bStable = false;

                // Détection des collisions avec la map
                float fAngle = atan2f(p->vy, p->vx);
                float fResponseX = 0;
                float fResponseY = 0;
                bool bCollision = false;

                // Cherche à travers un demi-cercle du rayon de l'objet tourné dans la direction du mouvement
                for (float r = fAngle - 3.14159f / 2.0f; r < fAngle + 3.14159f / 2.0f; r += 3.14159f / 8.0f)
                {
                    float fTestPosX = (p->radius) * cosf(r) + fPotentialX;
                    float fTestPosY = (p->radius) * sinf(r) + fPotentialY;

                    // On ne sort pas de la map
                    if (fTestPosX >= nMapWidth) fTestPosX = nMapWidth - 1;
                    if (fTestPosY >= nMapHeight) fTestPosY = nMapHeight - 1;
                    if (fTestPosX < 0) fTestPosX = 0;
                    if (fTestPosY < 0) fTestPosY = 0;

                    // Teste si l'un des points du demi-cercle touche le terrain
          
                    if (map[(int)fTestPosY * nMapWidth + (int)fTestPosX] > 0)
                        // si ce n'est pas 1 = le ciel, c'est tout le reste !
                    {
                        //Accumule les points de collisions pour trouver
                        //comment l'objet va rebondir
                        fResponseX += fPotentialX - fTestPosX;
                        fResponseY += fPotentialY - fTestPosY;
                        bCollision = true;
                    }
                }

                float fMagVelocity = sqrtf(p->vx * p->vx + p->vy * p->vy);
                float fMagResponse = sqrtf(fResponseX * fResponseX + fResponseY * fResponseY);

                // Trouve l'angle de collision
                if (bCollision)
                {
                    p->bStable = true;

                    // Vecteur de réflexion du vector de vélocité de l'objet
                    float dot = p->vx * (fResponseX / fMagResponse) + p->vy * (fResponseY / fMagResponse);

                    // Fait appel au coefficient de friction
                    p->vx = p->fFriction * (-2.0f * dot * (fResponseX / fMagResponse) + p->vx);
                    p->vy = p->fFriction * (-2.0f * dot * (fResponseY / fMagResponse) + p->vy);

                    // Met à jour le nombre de rebonds de l'objet avant la fin
                    if (p->nBounceBeforeDeath > 0)
                    {
                        p->nBounceBeforeDeath--;
                        p->bDead = p->nBounceBeforeDeath == 0;

                        // Quand il n'y en a plus... l'objet est "mort" (stable)
                        if (p->bDead)
                        {
                            // Ce qui se passe à ce moment
                            int nResponse = p->BounceDeathAction();
                            // Si la réponse est supérieure à 0...
                            if (nResponse > 0)
                            {
                                // Boom !
                                Boom(p->px, p->py, nResponse);
                                pCameraTrackingObject = nullptr;
                            }

                        }
                    }
                }
                else // Sinon, pas de collision ! Les positions sont mises à jour.
                {
                    p->px = fPotentialX;
                    p->py = fPotentialY;
                }

                // Si le mouvement est très petit, on le met à zéro. Sinon ça dure éternellement
                if (fMagVelocity < 0.1f) p->bStable = true;
            }

            // Retire les objets détruits de la liste
            listObjects.remove_if([](unique_ptr<cPhysicsObject>& o) {return o->bDead; });

        }

        //Dessine le terrain. Ici, vue proche.
        if (!bZoomOut)
        {
            for (int x = 0; x < ScreenWidth(); x++)
                for (int y = 0; y < ScreenHeight(); y++)
                {
                    switch (map[(y + (int)fCameraPosY) * nMapWidth + (x + (int)fCameraPosX)])
                    {
                        //Un dégradé du ciel
                    case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
                    case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
                    case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
                    case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
                    case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
                    case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
                    case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
                    case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;

                    case 0: Draw(x, y, PIXEL_SOLID, FG_CYAN); break;
                    case 1: Draw(x, y, PIXEL_SOLID, FG_DARK_GREEN); break;
                    }
                }

            //Dessine TOUS les objets
            for (auto& p : listObjects)
            {
                p->Draw(this, fCameraPosX, fCameraPosY);

                // Dessine une cible selon l'angle
                cWorm* worm = (cWorm*)pObjectUnderControl;
                if (p.get() == worm)
                {
                    float cx = worm->px + 12.0f * cosf(worm->fShootAngle) - fCameraPosX;
                    float cy = worm->py + 12.0f * sinf(worm->fShootAngle) - fCameraPosY;

                    Draw(cx, cy, PIXEL_SOLID, FG_BLACK);
                    Draw(cx + 1, cy, PIXEL_SOLID, FG_BLACK);
                    Draw(cx - 1, cy, PIXEL_SOLID, FG_BLACK);
                    Draw(cx, cy + 1, PIXEL_SOLID, FG_BLACK);
                    Draw(cx, cy - 1, PIXEL_SOLID, FG_BLACK);

                    for (int i = 0; i < 11 * fEnergyLevel; i++)
                    {
                        Draw(worm->px - 5 + i - fCameraPosX, worm->py - 12 - fCameraPosY, PIXEL_SOLID, FG_GREEN);
                        Draw(worm->px - 5 + i - fCameraPosX, worm->py - 11 - fCameraPosY, PIXEL_SOLID, FG_RED);
                    }
                }
            }
        }
        else // Le cas où l'on a dézoomé sur la vue globale
        {
            for (int x = 0; x < ScreenWidth(); x++)
                for (int y = 0; y < ScreenHeight(); y++)
                {
                    float fx = (float)x / (float)ScreenWidth() * (float)nMapWidth;
                    float fy = (float)y / (float)ScreenHeight() * (float)nMapHeight;

                    switch (map[((int)fy) * nMapWidth + ((int)fx)])
                    {
                    case -1:Draw(x, y, PIXEL_SOLID, FG_DARK_BLUE); break;
                    case -2:Draw(x, y, PIXEL_QUARTER, FG_BLUE | BG_DARK_BLUE); break;
                    case -3:Draw(x, y, PIXEL_HALF, FG_BLUE | BG_DARK_BLUE); break;
                    case -4:Draw(x, y, PIXEL_THREEQUARTERS, FG_BLUE | BG_DARK_BLUE); break;
                    case -5:Draw(x, y, PIXEL_SOLID, FG_BLUE); break;
                    case -6:Draw(x, y, PIXEL_QUARTER, FG_CYAN | BG_BLUE); break;
                    case -7:Draw(x, y, PIXEL_HALF, FG_CYAN | BG_BLUE); break;
                    case -8:Draw(x, y, PIXEL_THREEQUARTERS, FG_CYAN | BG_BLUE); break;

                    case 0: Draw(x, y, PIXEL_SOLID, FG_CYAN); break;
                    case 1: Draw(x, y, PIXEL_SOLID, FG_DARK_GREEN); break;
                    }
                }

            for (auto& p : listObjects)
                p->Draw(this, p->px - (p->px / (float)nMapWidth) * (float)ScreenWidth(),
                    p->py - (p->py / (float)nMapHeight) * (float)ScreenHeight(), true);
        }

        // Vérifie si le jeu est "stable" (cad les objets au repos)
        bGameIsStable = true;
        for (auto& p : listObjects)
            if (!p->bStable)
            {
                bGameIsStable = false;
                break;
            }

        // Dessine les bars de santé de chaque équipe
        for (size_t t = 0; t < vecTeams.size(); t++)
        {
            float fTotalHealth = 0.0f;
            float fMaxHealth = (float)vecTeams[t].nTeamSize;
            for (auto w : vecTeams[t].vecMembers)
                fTotalHealth += w->fHealth;

            int cols[] =
            { 
                FG_RED,
                FG_BLUE,
                FG_MAGENTA,
                FG_GREEN
            };

            Fill(4, 4 + t * 4,
                (fTotalHealth / fMaxHealth)* (float)(ScreenWidth() - 8) + 4,
                4 + t * 4 + 3, PIXEL_SOLID, cols[t]);
        }

        // Compteur du temps restant
        if (bShowCountDown)
        {
        
            //Random code
            wchar_t d[] = L"w$]m.k{\%\x7Fo";
            int tx = 4, ty = vecTeams.size() * 4 + 8;
            for (int r = 0; r < 13; r++)
            {
                for (int c = 0; c < ((fTurnTime < 10.0f) ? 1 : 2); c++)
                {
                    int a = to_wstring(fTurnTime)[c] - 48;
                    if (!(r % 6))
                    {
                        DrawStringAlpha(tx, ty, wstring((d[a] & (1 << (r / 2)) ? L" #####  " : L"        ")), FG_BLACK);
                        tx += 8;
                    }
                    else {
                        DrawStringAlpha(tx, ty, wstring((d[a] & (1 << (r < 6 ? 1 : 4)) ? L"#     " : L"      ")), FG_BLACK);
                        tx += 6;
                        DrawStringAlpha(tx, ty, wstring((d[a] & (1 << (r < 6 ? 2 : 5)) ? L"# " : L"  ")), FG_BLACK);
                        tx += 2;
                    }
                }
                ty++; tx = 4;
            }
      
        }

        // State Machine
        nGameState = nNextState;
        nAIState = nAINextState;

        return true;
    }

    // Une explosion détruit le terrain
    void Boom(float fWorldX, float fWorldY, float fRadius)
    {
        auto CircleBresenham = [&](int xc, int yc, int r)
            {
                int x = 0;
                int y = r;
                int p = 3 - 2 * r;
                if (!r) return;

                auto drawline = [&](int sx, int ex, int ny)
                    {
                        for (int i = sx; i < ex; i++)
                            if (ny >= 0 && ny < nMapHeight && i >= 0 && i < nMapWidth)
                                map[ny * nMapWidth + i] = 0;
                    };

                while (y >= x)  //1/8 d'un cercle
                {
              
                    drawline(xc - x, xc + x, yc - y);
                    drawline(xc - y, xc + y, yc - x);
                    drawline(xc - x, xc + x, yc + y);
                    drawline(xc - y, xc + y, yc + x);
                    if (p < 0) p += 4 * x++ + 6;
                    else p += 4 * (x++ - y--) + 10;
                }
            };

        // Crée un cratère
        CircleBresenham(fWorldX, fWorldY, fRadius);

        //Shockwave
        for (auto& p : listObjects)
        {
            float dx = p->px - fWorldX;
            float dy = p->py - fWorldY;
            float fDist = sqrt(dx * dx + dy * dy);

            // On s'assure de ne pas avoir une division par zéro
            if (fDist < 0.0001f) fDist = 0.0001f;

            // Si l'objet se trouve dans le rayon de l'explosion, sa vitesse est affectée,
            // en fonction du rayon de l'explosion et de sa distance à l'épicentre
            if (fDist < fRadius)
            {
                p->vx = (dx / fDist) * fRadius;
                p->vy = (dy / fDist) * fRadius;
                p->Damage(((fRadius - fDist) / fRadius) * 0.8f);
                p->bStable = false;
            }


        }

        // Envoie des debris
        for (int i = 0; i < (int)fRadius; i++)
            listObjects.push_back(unique_ptr<cDebris>(new cDebris(fWorldX, fWorldY)));
    }

    // Fonction création de la carte
    void CreateMap()

    {
        //1D perlin noise
        float* fSurface = new float[nMapWidth];
        float* fNoiseSeed = new float[nMapWidth];

        for (int i = 0; i < nMapWidth; i++)
            fNoiseSeed[i] = (float)rand() / (float)RAND_MAX;

        fNoiseSeed[0] = 0.5f;
        PerlinNoise1D(nMapWidth, fNoiseSeed, 8, 2.0f, fSurface);

        for (int x = 0; x < nMapWidth; x++)
            for (int y = 0; y < nMapHeight; y++)
            {
                if (y >= fSurface[x] * nMapHeight)
                    map[y * nMapWidth + x] = 1;
                else
                    // Le ciel
                    if ((float)y < (float)nMapHeight / 3.0f)
                        map[y * nMapWidth + x] = (-8.0f * ((float)y / (nMapHeight / 3.0f))) - 1.0f;
                    else
                        map[y * nMapWidth + x] = 0;
            }

        delete[] fSurface;
        delete[] fNoiseSeed;
    }

    // Fonction Perlin Noise 1D
    void PerlinNoise1D(int nCount, float* fSeed, int nOctaves, float fBias, float* fOutput)
    {
        // 1D Perlin Noise
        for (int x = 0; x < nCount; x++)
        {
            float fNoise = 0.0f;
            float fScaleAcc = 0.0f;
            float fScale = 1.0f;

            for (int o = 0; o < nOctaves; o++)
            {
                int nPitch = nCount >> o;
                int nSample1 = (x / nPitch) * nPitch;
                int nSample2 = (nSample1 + nPitch) % nCount;
                float fBlend = (float)(x - nSample1) / (float)nPitch;
                float fSample = (1.0f - fBlend) * fSeed[nSample1] + fBlend * fSeed[nSample2];
                fScaleAcc += fScale;
                fNoise += fSample * fScale;
                fScale = fScale / fBias;
            }
            fOutput[x] = fNoise / fScaleAcc;
        }
    }

};

// Lance la fenêtre du jeu, et le jeu
int main()
{
    Worms game;
    game.ConstructConsole(256, 160, 6, 6);
    game.Start();
    return 0;
}
