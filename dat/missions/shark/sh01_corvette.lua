--[[
<?xml version='1.0' encoding='utf8'?>
<mission name="Sharkman Is Back">
 <flags>
  <unique />
 </flags>
 <avail>
  <priority>3</priority>
  <done>A Shark Bites</done>
  <chance>10</chance>
  <location>Bar</location>
  <faction>Dvaered</faction>
  <faction>Empire</faction>
  <faction>Frontier</faction>
  <faction>Goddard</faction>
  <faction>Independent</faction>
  <faction>Sirius</faction>
  <faction>Soromid</faction>
  <faction>Traders Guild</faction>
  <faction>Za'lek</faction>
 </avail>
 <notes>
  <campaign>Nexus show their teeth</campaign>
 </notes>
</mission>
--]]
--[[
   This is the second mission of the Shark's teeth campaign. The player has to take part to a fake battle.

   Stages :
   0) Way to Toaxis
   1) Battle
   2) Going to Darkshed
--]]
local pir = require "common.pirate"
local fmt = require "format"
local shark = require "common.shark"


function create ()
   --Change here to change the planet and the system
   bsyname = "Toaxis"
   psyname = "Alteris"
   pplname = "Darkshed"
   --System neighbouring Toaxis with zero pirate presence due to a "Virtual Pirate Unpresence" asset
   esyname = "Ingot"
   battlesys = system.get(bsyname)
   paysys = system.get(psyname)
   paypla = planet.getS(pplname)
   escapesys = system.get(esyname)

   if not misn.claim(battlesys) then
      misn.finish(false)
   end

   misn.setNPC(_("Arnold Smith"), "neutral/unique/arnoldsmith.webp", _([[The Nexus employee seems to be looking for pilots. Maybe he has an other task for you.]]))
end

function accept()

   stage = 0
   reward = 750e3

   if tk.yesno(_("Nexus Shipyards needs you"), _([["I have another job for you. The Baron was unfortunately not as impressed as we hoped. So we need a better demonstration, and we think we know what to do: we're going to demonstrate that the Lancelot, our higher-end fighter design, is more than capable of defeating destroyer class ships.
    "Now, one small problem we face is that pirates almost never use destroyer class ships; they tend to stick to fighters, corvettes, and cruisers. More importantly, actually sending a fighter after a Destroyer is exceedingly dangerous, even if we could find a pirate piloting one. So we have another plan: we want someone to pilot a destroyer class ship and just let another pilot disable them with ion cannons.
    "What do you say? Are you interested?"]]):format(battlesys:name(), fmt.number(reward/2))) then
      misn.accept()
      tk.msg(_("Wonderful"), _([["Great! Go and meet our pilot in %s. After the job is done, meet me on %s in the %s system."]]):format(battlesys:name(), paypla:name(), paysys:name()))

      misn.setTitle(_("Sharkman is back"))
      misn.setReward(fmt.credits(reward/2))
      misn.setDesc(_("Nexus Shipyards wants you to fake a loss against a Lancelot while piloting a Destroyer class ship."))
      misn.osdCreate(_("Sharkman Is Back"), {
         _("Jump in %s with a destroyer class ship and let the Lancelot disable you"):format(battlesys:name()),
         _("Go to %s in %s to collect your pay"):format(paypla:name(), paysys:name()),
      })
      misn.osdActive(1)

      marker = misn.markerAdd(battlesys, "low")

      jumpouthook = hook.jumpout("jumpout")
      landhook = hook.land("land")
      enterhook = hook.enter("enter")
   else
      tk.msg(_("Sorry, not interested"), _([["OK, that's alright."]]))
      misn.finish(false)
   end
end

function jumpout()
   if stage == 1 then --player trying to escape
      player.msg( "#r" .. _("MISSION FAILED: You ran away.") .. "#0" )
      misn.finish(false)
   end
end

function land()
   if stage == 1 then --player trying to escape
      player.msg( "#r" .. _("MISSION FAILED: You ran away.") .. "#0" )
      misn.finish(false)
   end
   if stage == 2 and planet.cur() == paypla then
      tk.msg(_("Reward"), _([[As you land, you see Arnold Smith waiting for you. He explains that the Baron was so impressed by the battle that he signed an updated contract with Nexus Shipyards, solidifying Nexus as the primary supplier of ships for his fleet. As a reward, they give you twice the sum of credits they promised to you.]]))
      pir.reputationNormalMission(rnd.rnd(2,3))
      player.pay(reward)
      misn.osdDestroy()
      hook.rm(enterhook)
      hook.rm(landhook)
      hook.rm(jumpouthook)
      shark.addLog( _([[You helped Nexus Shipyards fake a demonstration by allowing a Lancelot to disable your Destroyer-class ship.]]) )
      misn.finish(true)
   end
end

function enter()

   playerclass = player.pilot():ship():class()
   --Jumping in Toaxis for the battle with a destroyer class ship
   if system.cur() == battlesys and stage == 0 and playerclass == "Destroyer" then
      pilot.clear()
      pilot.toggleSpawn( false )

      hook.timer(2.0,"lets_go")
   end
end

function lets_go()
   -- spawns the Shark
   sharkboy = pilot.add( "Lancelot", "Mercenary", system.get("Raelid"), nil, {ai="baddie_norun"} )
   sharkboy:setHostile()
   sharkboy:setHilight()

   --The shark becomes nice outfits
   sharkboy:outfitRm("all")
   sharkboy:outfitRm("cores")

   sharkboy:outfitAdd("S&K Light Combat Plating")
   sharkboy:outfitAdd("Milspec Orion 3701 Core System")
   sharkboy:outfitAdd("Tricon Zephyr II Engine")

   sharkboy:outfitAdd("Reactor Class I",2)

   sharkboy:outfitAdd("Heavy Ion Cannon")
   sharkboy:outfitAdd("Ion Cannon",3)

   sharkboy:setHealth(100,100)
   sharkboy:setEnergy(100)
   sharkboy:setFuel(true)
   stage = 1

   shark_dead_hook = hook.pilot( sharkboy, "death", "shark_dead" )
   disabled_hook = hook.pilot( player.pilot(), "disable", "disabled" )
end

function shark_dead()  --you killed the shark
   player.msg( "#r" .. _("MISSION FAILED: You destroyed the Lancelot.") .. "#0" )
   misn.finish(false)
end

function disabled(pilot, attacker)
   if attacker == sharkboy then
      stage = 2
      misn.osdActive(2)
      misn.markerRm(marker)
      marker2 = misn.markerAdd(paysys, "low")
      pilot.toggleSpawn( true )
   end
   sharkboy:control()
   --making sure the shark doesn't continue attacking the player
   sharkboy:hyperspace(escapesys)
   sharkboy:setNoDeath(true)

   -- Clean up now unneeded hooks
   hook.rm(shark_dead_hook)
   hook.rm(disabled_hook)
end
