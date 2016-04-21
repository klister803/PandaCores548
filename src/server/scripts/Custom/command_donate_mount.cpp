#include "ScriptPCH.h"
#include "Player.h"


class check_mount_display : public PlayerScript
{
    public:
        check_mount_display() : PlayerScript("check_mount_display") {}

        void OnLogin(Player *player)  // проверка при входе
        {
              if (player->HasAura(121805)) // Fly - fly
               {          
                  player->RemoveAurasDueToSpell(121805);
               }  
              if (player->HasAura(58819))
              {
                 player->RemoveAurasDueToSpell(58819);
              }
        }
};   


void AddSC_donate_mount()
{
    new check_mount_display();
}