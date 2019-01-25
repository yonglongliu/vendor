function testTapTigger(hTfa)
% must be initialize before with following command:

hTfa = initTapLiteIP ;

[~, Address, ~, ~] = hTfa.getVarAddress('statePR->prevIdTap');
statePRAdr = str2num(Address{1});  %starting address for whole vector

[~, Address, ~, ~] = hTfa.getVarAddress('statePR->newTapFlag');
newTapFlagAdr = str2num(Address{1});  %flag to check if new Tap is detect

statePRSize = newTapFlagAdr - statePRAdr + 1 ;

[~, Address, ~, ~] = hTfa.getVarAddress('statePR->flagTapPattern');
flagTapPatternOff = str2num(Address{1}) - statePRAdr + 1; % offset in whole statePR vector

[~, Address, ~, ~] = hTfa.getVarAddress('statePR->countNumberGaps');
countNumberGapsOff = str2num(Address{1}) - statePRAdr + 1; % offset in whole statePR vector

[~, Address, ~, ~] = hTfa.getVarAddress('statePR->tapPatternTime');
tapPatternTimeOff = str2num(Address{1}) - statePRAdr + 1; % offset in whole statePR vector

[~, Address, ~, ~] = hTfa.getVarAddress('statePR->getGlitchs');
getGlitchsOff = str2num(Address{1}) - statePRAdr + 1; % offset in whole statePR vector

while(1)
    
    java.lang.Thread.sleep(40); %40 ms delay
    newTap = hTfa.ReadMemory(newTapFlagAdr, 1); %read new Tap flag
    if (newTap)
        statePR = hTfa.ReadMemory(statePRAdr, statePRSize); %read whole vector
        hTfa.WriteMemory(newTapFlagAdr, 0); %clear the flag in XMEM
        disp(['New Tap at: ' datestr(now)])
        flagTapPattern = statePR(flagTapPatternOff);
        countNumberGaps = statePR(countNumberGapsOff);
        getGlitchs = statePR(getGlitchsOff);
        disp(['flagTapPattern: ' num2str(flagTapPattern)]);
        disp(['countNumberGaps: ' num2str(countNumberGaps)]);
        disp(['Glitches: ' num2str(getGlitchs)]);        
        if countNumberGaps
            tapPatternTime = statePR(tapPatternTimeOff:tapPatternTimeOff+countNumberGaps-1);
            disp(['tapPatternTime: ' num2str(tapPatternTime)]);
        end
    end
end
