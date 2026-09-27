-- CoAReporterUI: form logic with dropdown pickers (no shift-click needed).
-- Quest: dropdown lists the quest log (ID + title).
-- NPC: dropdown lists current target + nearby mouseover NPC (GUID -> NPC ID).
-- Item: dropdown lists bag items (with counts and quality colors).
-- Spell: dropdown lists the player's spellbook spells.
-- Talent: dropdown lists learned talents with rank (tab/idx captured internally).
-- Other: no picker, position only (server adds class/level/map/position/revision).

CoAReporterUI = {}
local state = { category = "quest", questIndex = nil, npcGUID = nil, itemBag = nil, itemSlot = nil,
                spellID = nil, talent = nil }

local CATEGORY_LABEL = {
    quest = "Which quest?",
    npc = "Which NPC?",
    item = "Which bag item?",
    spell = "Which spell?",
    talent = "Which talent?",
    other = "No target needed",
}

local frame -- set in Init

-- ------------------------------------------------------------ dropdown data

local function DropdownQuests(level)
    local info = UIDropDownMenu_CreateInfo()
    local n = GetNumQuestLogEntries()
    local count = 0
    for i = 1, n do
        local title, _, _, isHeader = GetQuestLogTitle(i)
        if not isHeader and title then
            local link = GetQuestLink and GetQuestLink(i)
            local qid = link and tonumber(string.match(link, "quest:(%d+)"))
                or select(8, GetQuestLogTitle(i))
            if qid then
                info.text = qid .. " - " .. title
                info.value = i
                info.checked = state.questIndex == i
                info.func = function()
                    state.questIndex = i
                    UIDropDownMenu_SetText(frame.Picker, qid .. " - " .. title)
                end
                UIDropDownMenu_AddButton(info, level)
                count = count + 1
            end
        end
    end
    if count == 0 then
        info.text = "(no quests in log)"
        info.notClickable = true
        info.func = nil
        UIDropDownMenu_AddButton(info, level)
    end
end

local function DropdownNPCs(level)
    local info = UIDropDownMenu_CreateInfo()
    local added = 0
    local function AddNPC(guid, label)
        if not guid or added >= 12 then return end
        local _, _, _, _, _, npcid = strsplit("-", guid)
        if not npcid then return end
        info.text = label .. " [" .. npcid .. "]"
        info.value = guid
        info.checked = state.npcGUID == guid
        info.func = function()
            state.npcGUID = guid
            UIDropDownMenu_SetText(frame.Picker, label .. " [" .. npcid .. "]")
        end
        UIDropDownMenu_AddButton(info, level)
        added = added + 1
    end
    if UnitExists("target") and UnitIsNPC and UnitIsNPC("target") ~= false then
        AddNPC(UnitGUID("target"), (UnitName("target") or "?") .. " (target)")
    elseif UnitExists("target") then
        AddNPC(UnitGUID("target"), (UnitName("target") or "?") .. " (target)")
    end
    -- mouseover fallback (hover NPC without targeting)
    if UnitExists("mouseover") then
        AddNPC(UnitGUID("mouseover"), (UnitName("mouseover") or "?") .. " (mouseover)")
    end
    if added == 0 then
        info.text = "(target or hover an NPC first)"
        info.notClickable = true
        info.func = nil
        UIDropDownMenu_AddButton(info, level)
    end
end

local function DropdownItems(level)
    local info = UIDropDownMenu_CreateInfo()
    local added = 0
    for bag = 0, NUM_BAG_FRAMES do
        for slot = 1, GetContainerNumSlots(bag) do
            local link = GetContainerItemLink(bag, slot)
            if link then
                local id = tonumber(string.match(link, "item:(%d+)"))
                if id then
                    local _, count = GetContainerItemInfo(bag, slot)
                    local name = string.match(link, "%[(.*)%]") or "?"
                    local _, _, quality = GetItemInfo(id)
                    local b, g, r = GetItemQualityColor and GetItemQualityColor(quality or 1)
                    info.text = name .. (count and count > 1 and (" x" .. count) or "")
                    info.value = bag * 100 + slot
                    info.checked = state.itemBag == bag and state.itemSlot == slot
                    if b then info.colorCode = "|cff" .. string.format("%02x%02x%02x", r * 255, g * 255, b * 255) end
                    info.func = function()
                        state.itemBag, state.itemSlot = bag, slot
                        state.itemID = id
                        UIDropDownMenu_SetText(frame.Picker, name)
                    end
                    UIDropDownMenu_AddButton(info, level)
                    added = added + 1
                    if added >= 60 then return end
                end
            end
        end
    end
    if added == 0 then
        info.text = "(bag empty)"
        info.notClickable = true
        info.func = nil
        UIDropDownMenu_AddButton(info, level)
    end
end

local function DropdownSpells(level)
    local info = UIDropDownMenu_CreateInfo()
    local added = 0
    local tabs = GetNumSpellTabs and GetNumSpellTabs() or 1
    for tab = 1, tabs do
        local _, _, offset, num = GetSpellTabInfo(tab)
        offset = offset or 0
        num = num or GetNumSpellTabSpells and GetNumSpellTabSpells() or 0
        for i = offset + 1, offset + num do
            local name, rank, sid = GetSpellBookItemName(i, BOOKTYPE_SPELL)
            if name and sid then
                info.text = name .. (rank and rank ~= "" and (" (" .. rank .. ")") or "")
                info.value = sid
                info.checked = state.spellID == sid
                info.func = function()
                    state.spellID = sid
                    UIDropDownMenu_SetText(frame.Picker, name)
                end
                UIDropDownMenu_AddButton(info, level)
                added = added + 1
                if added >= 80 then return end
            end
        end
    end
    if added == 0 then
        info.text = "(no spells found)"
        info.notClickable = true
        info.func = nil
        UIDropDownMenu_AddButton(info, level)
    end
end

local function DropdownTalents(level)
    local info = UIDropDownMenu_CreateInfo()
    local added = 0
    for tab = 1, GetNumTalentTabs() do
        info.text = GetTalentTabInfo(tab)
        info.isTitle = true
        info.notClickable = true
        UIDropDownMenu_AddButton(info, level)
        for idx = 1, GetNumTalents(tab) do
            local name, icon, _, _, rank = GetTalentInfo(tab, idx, false)
            if name and rank and rank > 0 then
                info.text = name .. " (" .. rank .. "/" .. 5 .. ")"
                info.isTitle = false
                info.notClickable = false
                info.value = tab * 100 + idx
                info.checked = state.talent and state.talent.tab == tab and state.talent.idx == idx
                info.func = function()
                    state.talent = { tab = tab, idx = idx }
                    UIDropDownMenu_SetText(frame.Picker, name .. " (rank " .. rank .. ")")
                end
                UIDropDownMenu_AddButton(info, level)
                added = added + 1
            end
        end
    end
    if added == 0 then
        info.text = "(no talents learned)"
        info.isTitle = false
        info.notClickable = true
        info.func = nil
        UIDropDownMenu_AddButton(info, level)
    end
end

local PICKERS = {
    quest = DropdownQuests,
    npc = DropdownNPCs,
    item = DropdownItems,
    spell = DropdownSpells,
    talent = DropdownTalents,
}

-- -------------------------------------------------------------------- frame

function CoAReporterUI_Init(f)
    local name = f:GetName()
    frame = f
    -- explicit _G lookups: this client doesn't expose XML children as parent fields
    frame.Send = _G[name .. "Send"]
    frame.Check = _G[name .. "Check"]
    frame.TitleBox = _G[name .. "TitleBox"]
    frame.Behavior = _G[name .. "Behavior"]
    frame.Expected = _G[name .. "Expected"]
    frame.Status = _G[name .. "Status"]
    frame.Picker = _G[name .. "Picker"] or _G[name .. "PickerRowPicker"]
    if not frame.Picker then
        -- fallback: find the dropdown among PickerRow children
        local row = _G[name .. "PickerRow"]
        if row then
            for _, child in ipairs({ row:GetChildren() }) do
                local cn = child:GetName()
                if cn and string.find(cn, "Picker$") then frame.Picker = child end
            end
        end
    end
    frame.PickerLabel = _G[name .. "PickerRowPickerLabel"]
    local cats = { "Quest", "NPC", "Item", "Spell", "Talent", "Other" }
    for _, c in ipairs(cats) do
        local b = _G[name .. "CategoryRow" .. c]
        b:SetScript("OnClick", function()
            state.category = string.lower(c)
            CoAReporterUI_Refresh()
        end)
    end
    if frame.Send then frame.Send:SetScript("OnClick", CoAReporterUI_Send) end
    if frame.Check then frame.Check:SetScript("OnClick", CoAReporterUI_Check) end
    -- register slash commands first so they exist even if later setup fails
    SLASH_COAREPORTER1 = "/coareport"
    SLASH_COAREPORTER2 = "/coabug"
    SlashCmdList["COAREPORTER"] = function() frame:Show() end
    CoAReporterUI_CreateMinimapButton()
    CoAReporterUI_HideTalkToGM()
    if frame.Picker then
        UIDropDownMenu_Initialize(frame.Picker, function(self, level)
            local picker = PICKERS[state.category]
            if picker then picker(level) end
        end)
    end
end

-- minimap bug icon (draggable around the minimap, click opens the report form)
function CoAReporterUI_CreateMinimapButton()
    local b = CreateFrame("Button", "CoAReporterMinimapButton", Minimap)
    b:SetFrameStrata("MEDIUM")
    b:SetSize(32, 32)
    b:SetFrameLevel(8)
    b:SetHighlightTexture("Interface\\Minimap\\UI-Minimap-ZoomButton-Highlight")
    local overlay = b:CreateTexture(nil, "OVERLAY")
    overlay:SetSize(53, 53)
    overlay:SetTexture("Interface\\Minimap\\MiniMap-TrackingBorder")
    overlay:SetPoint("TOPLEFT")
    local bg = b:CreateTexture(nil, "BACKGROUND")
    bg:SetSize(20, 20)
    bg:SetTexture("Interface\\Minimap\\UI-Minimap-Background")
    bg:SetPoint("TOPLEFT", 7, -5)
    local icon = b:CreateTexture(nil, "ARTWORK")
    icon:SetSize(16, 16)
    icon:SetTexture("Interface\\Icons\\INV_Misc_Bug_01")
    icon:SetPoint("TOPLEFT", 9, -7)
    b.icon = icon
    b:SetMovable(true)
    b:RegisterForClicks("AnyUp")
    b:RegisterForDrag("LeftButton")
    b:SetScript("OnDragStart", function(self)
        self:StartMoving()
        self:SetScript("OnUpdate", CoAReporterUI_MinimapDrag)
    end)
    b:SetScript("OnDragStop", function(self)
        self:SetScript("OnUpdate", nil)
        self:StopMovingOrSizing()
        local mx, my = Minimap:GetCenter()
        local bx, by = self:GetCenter()
        local angle = math.deg(math.atan2(by - my, bx - mx))
        CoAReporterDB.minimapAngle = angle
        self:SetMovable(false)
    end)
    b:SetScript("OnClick", function(self, button)
        if button == "RightButton" then
            self:ClearAllPoints()
            self:SetPoint("CENTER", Minimap, "CENTER", 82, -20)
        else
            if frame then frame:Show() end
        end
    end)
    b:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_BOTTOMLEFT")
        GameTooltip:SetText("CoA Bug Reporter")
        GameTooltip:AddLine("Left-click: report a bug", 1, 1, 1)
        GameTooltip:AddLine("Drag: move | Right-click: reset", 0.8, 0.8, 0.8)
        GameTooltip:Show()
    end)
    b:SetScript("OnLeave", function() GameTooltip:Hide() end)
    CoAReporterUI_PlaceMinimapButton(b)
end

function CoAReporterUI_PlaceMinimapButton(b)
    local angle = CoAReporterDB.minimapAngle or -90
    local x = math.cos(math.rad(angle)) * 82
    local y = math.sin(math.rad(angle)) * 82
    b:ClearAllPoints()
    b:SetPoint("CENTER", Minimap, "CENTER", x, y)
end

function CoAReporterUI_MinimapDrag(self)
    local mx, my = Minimap:GetCenter()
    local cx, cy = GetCursorPosition()
    local scale = Minimap:GetEffectiveScale()
    local bx, by = cx / scale, cy / scale
    local angle = math.deg(math.atan2(by - my, bx - mx))
    local x = math.cos(math.rad(angle)) * 82
    local y = math.sin(math.rad(angle)) * 82
    self:ClearAllPoints()
    self:SetPoint("CENTER", Minimap, "CENTER", x, y)
end

-- hide the native "Talk to GM" button in the client's built-in Help UI
function CoAReporterUI_HideTalkToGM()
    local function scan()
        local names = { "HelpFrameTicketOpenFrame", "HelpFrameOpenTicketFrame", "HelpFrameButton16" }
        for _, n in ipairs(names) do
            local f = _G[n]
            if f then f:Hide() end
        end
        -- generic sweep: any button whose text mentions GM inside the help frame
        if HelpFrame then
            for _, child in ipairs({ HelpFrame:GetChildren() }) do
                if child:IsObjectType("Button") then
                    local label = child.GetText and child:GetText()
                    if label and string.find(string.lower(label), "gm") then
                        child:Hide()
                        child:SetScript("OnShow", child.Hide)
                    end
                end
            end
        end
    end
    if HelpFrame then scan() end
    frame:HookScript("OnShow", scan)
end

function CoAReporterUI_Refresh()
    if not frame then return end
    if frame.Picker then UIDropDownMenu_SetText(frame.Picker, CATEGORY_LABEL[state.category] or "") end
    if frame.PickerLabel then frame.PickerLabel:SetText(CATEGORY_LABEL[state.category] or "") end
    if state.category == "npc" then state.npcGUID = nil end
    frame.TitleBox:SetText("")
    frame.Behavior:SetText("")
    frame.Expected:SetText("")
    frame.Status:SetText("")
end

-- getters used by CoAReporter.lua payload builder
function CoAReporterUI.GetSelectedQuest() return state.questIndex end
function CoAReporterUI.GetSelectedNPC() return state.npcGUID end
function CoAReporterUI.GetSelectedItemID() return state.itemID end
function CoAReporterUI.GetSelectedSpell() return state.spellID end
function CoAReporterUI.GetSelectedTalent() return state.talent end

function CoAReporterUI_Send()
    local title = frame.TitleBox:GetText()
    local behavior = frame.Behavior:GetText()
    local expected = frame.Expected:GetText()
    if not title or title == "" then
        frame.Status:SetText("Give the report a short title first.")
        return
    end
    local payload = CoAReporter_BuildPayload(state.category, title, behavior, expected)
    frame.Status:SetText("Uploading...")
    CoAReporter_Submit(payload, title)
end

function CoAReporterUI_Check()
    local pending = CoAReporterDB and CoAReporterDB.pending
    if not pending or not pending.id then
        frame.Status:SetText("No report in flight.")
        return
    end
    SendAddonMessage("COABUG\t", "Q|" .. pending.id .. "|", "WHISPER", UnitName("player"))
end

function CoAReporterUI.ReportError(code)
    if not frame then return end
    local msg = { cooldown = "Wait before sending another report.", busy = "Another upload in progress.",
        invalid = "Report rejected by server.", sequence = "Upload interrupted, try again.",
        missing = "Upload expired, try again.", queued = "Already queued.", storage = "Server storage error." }
    frame.Status:SetText(msg[code] or ("Error: " .. tostring(code)))
end

function CoAReporterUI.ReportStatus(status)
    if not frame then return end
    if status == "queued" then
        frame.Status:SetText("Report queued on server.")
        CoAReporter_Reset()
    elseif string.sub(status, 1, 8) == "created|" then
        frame.Status:SetText("GitHub issue #" .. string.sub(status, 9) .. " created.")
        CoAReporterDB.pending = nil
        CoAReporter_Reset()
    else
        frame.Status:SetText("Status: " .. tostring(status))
    end
end
