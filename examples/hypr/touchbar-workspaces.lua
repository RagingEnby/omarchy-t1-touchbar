-- Omarchy/Hyprland personal bindings for a programmable T1 Touch Bar.
-- Paste this reviewed block into ~/.config/hypr/bindings.lua.
-- The dfrd F13-F24 layer emits these extended keys.

for workspace = 1, 10 do
  local key = "F" .. tostring(workspace + 12)
  o.bind(
    key,
    "Touch Bar: workspace " .. tostring(workspace),
    hl.dsp.focus({ workspace = tostring(workspace) })
  )
end

o.bind(
  "F23",
  "Touch Bar: previous workspace",
  hl.dsp.focus({ workspace = "e-1" })
)

o.bind(
  "F24",
  "Touch Bar: next workspace",
  hl.dsp.focus({ workspace = "e+1" })
)
