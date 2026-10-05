$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$ProtocolPath = Join-Path $Root 'Shared/Protocol/protocol-v1.yaml'

function Assert-True {
    param(
        [Parameter(Mandatory = $true)][bool]$Condition,
        [Parameter(Mandatory = $true)][string]$Message
    )
    if (-not $Condition) {
        throw "Protocol validation failed: $Message"
    }
}

Assert-True (Test-Path $ProtocolPath) "protocol not found: $ProtocolPath"
$Text = Get-Content $ProtocolPath -Raw

Assert-True ($Text -match '(?m)^name:\s*FantasyProtocol\s*$') 'name must be FantasyProtocol'
Assert-True ($Text -match '(?m)^version:\s*1\s*$') 'version must be 1'
Assert-True ($Text -match '(?m)^transport:\s*tcp\s*$') 'transport must be tcp'
Assert-True ($Text -match '(?m)^endianness:\s*little\s*$') 'endianness must be little'

$RequiredEnvelope = @{
    magic = 'fixed_ascii'
    protocolVersion = 'uint16'
    messageType = 'uint16'
    payloadLength = 'uint32'
    sequence = 'uint32'
}

foreach ($Entry in $RequiredEnvelope.GetEnumerator()) {
    $Pattern = [regex]::Escape("- { name: $($Entry.Key), type: $($Entry.Value)")
    Assert-True ($Text -match $Pattern) "missing envelope field $($Entry.Key):$($Entry.Value)"
}
Assert-True ($Text -match 'name:\s*magic,\s*type:\s*fixed_ascii,\s*value:\s*FNTY') 'magic must be FNTY'

$MessagePattern = [regex]'(?ms)^  - id:\s*(?<id>\d+)\s*\r?\n    name:\s*(?<name>[A-Za-z][A-Za-z0-9]*)\s*\r?\n    direction:\s*(?<direction>[a-z_]+)\s*\r?\n    fields:\s*\r?\n(?<fields>(?:      - \{[^\r\n]+\}\s*\r?\n)+)'
$Matches = $MessagePattern.Matches($Text)
Assert-True ($Matches.Count -gt 0) 'no messages parsed'

$AllowedDirections = @('client_to_server', 'server_to_client', 'bidirectional')
$AllowedTypes = @('uint8', 'uint16', 'uint32', 'uint64', 'int16', 'int32', 'string', 'bytes', 'fixed_ascii')
$Ids = @{}
$Names = @{}
$MessageInfo = @{}

foreach ($Match in $Matches) {
    $Id = [int]$Match.Groups['id'].Value
    $Name = $Match.Groups['name'].Value
    $Direction = $Match.Groups['direction'].Value
    $FieldsBlock = $Match.Groups['fields'].Value

    Assert-True ($Id -ge 0 -and $Id -le 65535) "message '$Name' id out of uint16 range: $Id"
    Assert-True (-not $Ids.ContainsKey($Id)) "duplicate message id: $Id"
    Assert-True (-not $Names.ContainsKey($Name)) "duplicate message name: $Name"
    Assert-True ($AllowedDirections -contains $Direction) "invalid direction '$Direction' for message '$Name'"
    $Ids[$Id] = $Name
    $Names[$Name] = $Id

    $FieldMatches = [regex]::Matches($FieldsBlock, '- \{\s*name:\s*(?<name>[A-Za-z][A-Za-z0-9]*),\s*type:\s*(?<type>[a-z0-9_]+)(?:,\s*value:\s*[^}]+)?\s*\}')
    Assert-True ($FieldMatches.Count -gt 0) "message '$Name' has no fields"

    $FieldNames = @{}
    foreach ($FieldMatch in $FieldMatches) {
        $FieldName = $FieldMatch.Groups['name'].Value
        $FieldType = $FieldMatch.Groups['type'].Value
        Assert-True (-not $FieldNames.ContainsKey($FieldName)) "duplicate field '$FieldName' in message '$Name'"
        Assert-True ($AllowedTypes -contains $FieldType) "unknown field type '$FieldType' in message '$Name.$FieldName'"
        $FieldNames[$FieldName] = $FieldType
    }

    $MessageInfo[$Name] = @{
        id = $Id
        direction = $Direction
        fields = $FieldNames
    }
}

$RequiredMessages = @{
    Hello = 1
    HelloAck = 2
    LoginDev = 16
    LoginOk = 17
    EnterWorld = 32
    MapChunk = 33
    EntityAdd = 48
    EntityMove = 49
    EntityRemove = 50
    Error = 254
    Disconnect = 255
}
foreach ($Entry in $RequiredMessages.GetEnumerator()) {
    Assert-True ($Names.ContainsKey($Entry.Key)) "required message missing: $($Entry.Key)"
    Assert-True ([int]$Names[$Entry.Key] -eq [int]$Entry.Value) "message '$($Entry.Key)' must use id $($Entry.Value)"
}

Assert-True ($MessageInfo['Hello'].direction -eq 'client_to_server') 'Hello direction must be client_to_server'
Assert-True ($MessageInfo['HelloAck'].direction -eq 'server_to_client') 'HelloAck direction must be server_to_client'
Assert-True ($MessageInfo['EntityMove'].direction -eq 'bidirectional') 'EntityMove direction must be bidirectional during draft v1'
Assert-True ($MessageInfo['EnterWorld'].fields.ContainsKey('entityId')) 'EnterWorld.entityId is required'
Assert-True ($MessageInfo['MapChunk'].fields.ContainsKey('payload')) 'MapChunk.payload is required'

foreach ($Rule in @('authoritativeServer', 'versionHandshakeRequired', 'unknownMessageIsProtocolError', 'clientCannotAuthoritativelySetPosition')) {
    $RulePattern = "(?m)^  ${Rule}:\s*true\s*$"
    Assert-True ($Text -match $RulePattern) "rule '$Rule' must be true"
}

Write-Host "Fantasy Protocol v1 validation PASS. messages=$($Matches.Count) ids=unique names=unique"
