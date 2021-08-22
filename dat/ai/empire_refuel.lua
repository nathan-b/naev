require 'ai.empire_idle'

-- Settings
mem.aggressive = false
mem.defensive  = false
mem.distressmsg = _("Empire refuel ship under attack!")

function create ()
   -- Broke
   ai.setcredits( 0 )

   mem.loiter = 3 -- This is the amount of waypoints the pilot will pass through before leaving the system

   -- Finish up creation
   create_post()
end

function hail ()
   if mem.setuphail then return end

   -- Doesn't make sense to bribe
   mem.bribe_no = _("I'm out of here.")

   -- Override refuel chance
   mem.refuel = 0
   mem.refuel_msg = _([["Sure thing."]])

   mem.setuphail = true
end
