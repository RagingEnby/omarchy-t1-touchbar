-- Personal Omarchy/Hyprland bindings for the custom T1 Touch Bar home row.
-- Add this reviewed block to ~/.config/hypr/bindings.lua.
-- F13-F22 are emitted by workspace buttons 1-10.

for workspace = 1, 10 do
  local key = "F" .. tostring(workspace + 12)
  o.bind(
    key,
    "Touch Bar: workspace " .. tostring(workspace),
    hl.dsp.focus({ workspace = tostring(workspace) })
  )
end

-- Launch the native ChatGPT app, or focus its existing window.
o.bind(
  "F23",
  "Touch Bar: ChatGPT",
  { focus = "chatgpt", launch = "chatgpt" }
)

o.bind(
  "F24",
  "Touch Bar: Omarchy menu",
  "omarchy menu toggle root"
)
