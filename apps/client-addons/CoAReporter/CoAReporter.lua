-- CoAReporter: in-game bug reporter wired to the server's COABUG protocol
-- (modules/mod-ascension-compat/src/CoABugReportService.h).
-- Protocol: self addon-whisper prefix "COABUG\t". H=handshake, B=begin, D=data chunk,
-- C=commit, Q=status. 180-byte chunks, stop-and-wait acks, server adds class/level/map/pos.

local PREFIX = "COABUG\t"
local CHUNK = 180
local MAXPAYLOAD = 12000

local addon = CreateFrame("Frame", "CoAReporter")
addon:RegisterEvent("PLAYER_LOGIN")

CoAReporterDB = CoAReporterDB or { pending = nil }  -- pending = {id, payload, size, seq, title}

-- ---------------------------------------------------------------- protocol

local function SendAddon(msg)
    SendAddonMessage(PREFIX, msg, "WHISPER", UnitName("player"))
end

local requestID = nil
local waitingAcks = {}  -- seq -> callback

local frame = CreateFrame("Frame")
frame:RegisterEvent("CHAT_MSG_ADDON")
frame:SetScript("OnEvent", function(_, _, prefix, message, _, sender)
    if prefix ~= PREFIX or sender ~= UnitName("player") then return end
    -- reply format: H|0 | H|1 | A|<id>|<seq> | S|<id>|<status> | E|<id>|<code>
    local op, id, arg = strsplit("|", message)
    if op == "A" and id == requestID and waitingAcks[tonumber(arg)] then
        local cb = waitingAcks[tonumber(arg)]
        waitingAcks[tonumber(arg)] = nil
        cb()
    elseif op == "E" and id == requestID then
        if CoAReporterUI then CoAReporterUI.ReportError(arg) end
        wipe(waitingAcks)
    elseif op == "S" and id == requestID then
        if CoAReporterUI then CoAReporterUI.ReportStatus(arg) end
    end
end)

local function ChunkLoop(id, payload, seq, size)
    if seq * CHUNK >= size then
        SendAddon("C|" .. id)
        return
    end
    local data = string.sub(payload, seq * CHUNK + 1, (seq + 1) * CHUNK)
    waitingAcks[seq + 1] = function()
        ChunkLoop(id, payload, seq + 1, size)
    end
    SendAddon("D|" .. id .. "|" .. (seq + 1) .. "|" .. data)
end

function CoAReporter_Submit(payload, title, onDone, onError)
    if #payload > MAXPAYLOAD then
        payload = string.sub(payload, 1, MAXPAYLOAD)
    end
    local size = #payload
    if requestID then return end
    requestID = string.format("%08x%08x%08x%08x", math.random(0, 2^31-1), math.random(0, 2^31-1),
        math.random(0, 2^31-1), math.random(0, 2^31-1))
    CoAReporterDB.pending = { id = requestID, payload = payload, size = size, title = title }
    -- begin upload: B|<id>|<size> -> server answers A|<id>|0, then data chunks flow
    waitingAcks[0] = function()
        ChunkLoop(requestID, payload, 0, size)
    end
    SendAddon("B|" .. requestID .. "|" .. size)
end

function CoAReporter_Reset()
    requestID = nil
    wipe(waitingAcks)
end

-- Handshake on demand from the UI
function CoAReporter_Handshake()
    requestID = "handshake"
    SendAddon("H|1")
end

-- ---------------------------------------------------------------- payload

local function Pos()
    local map = GetCurrentMapDungeonLevel and GetCurrentMapDungeonLevel() or 0
    SetMapToCurrentZone()
    local x, y = GetPlayerMapPosition("player")
    local zone = GetZoneText()
    return string.format("%s (map %d) at %.1f, %.1f", zone, GetCurrentMapAreaID(), x * 100, y * 100)
end

local CATEGORY_FIELDS = {
    quest = function()
        local qi = CoAReporterUI and CoAReporterUI.GetSelectedQuest()
        if qi then
            local title, _, _, _, _, _, _, qid = GetQuestLogTitle(qi)
            return string.format("Quest ID: %d\nQuest: %s", qid or 0, title or "?")
        end
        return nil
    end,
    npc = function()
        local guid = CoAReporterUI and CoAReporterUI.GetSelectedNPC()
        if guid then
            local _, _, _, _, _, npcid = strsplit("-", guid)
            local unit = UnitExists("target") and UnitGUID("target") == guid and "target"
                or (UnitExists("mouseover") and UnitGUID("mouseover") == guid and "mouseover")
            local name = unit and UnitName(unit) or "?"
            if npcid then
                return string.format("NPC ID: %s\nNPC: %s", npcid, name)
            end
        end
        return nil
    end,
    item = function()
        local id = CoAReporterUI and CoAReporterUI.GetSelectedItemID()
        if id then
            local name = GetItemInfo(id)
            return string.format("Item ID: %d\nItem: %s", id, name or "?")
        end
        return nil
    end,
    spell = function()
        local id = CoAReporterUI and CoAReporterUI.GetSelectedSpell()
        if id then
            local name = GetSpellInfo(id)
            return string.format("Spell ID: %d\nSpell: %s", id, name or "?")
        end
        return nil
    end,
    talent = function()
        local id = CoAReporterUI and CoAReporterUI.GetSelectedTalent()
        if id then
            local name, _, _, _, _, rank = GetTalentInfo(id.tab, id.idx, false)
            return string.format("Talent: %s (tab %d, index %d, rank %d)", name or "?", id.tab, id.idx, rank or 0)
        end
        return nil
    end,
}

function CoAReporter_BuildPayload(category, title, behavior, expected)
    local parts = {}
    local ident = CATEGORY_FIELDS[category] and CATEGORY_FIELDS[category]()
    if ident then table.insert(parts, ident) end
    table.insert(parts, string.format("Category: %s", category))
    table.insert(parts, string.format("Title: %s", title))
    table.insert(parts, string.format("Location: %s", Pos()))
    if behavior and behavior ~= "" then table.insert(parts, "Observed behavior:\n" .. behavior) end
    if expected and expected ~= "" then table.insert(parts, "Expected behavior:\n" .. expected) end
    return table.concat(parts, "\n\n")
end
