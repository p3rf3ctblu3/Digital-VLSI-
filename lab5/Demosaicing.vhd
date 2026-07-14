library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity Demosaicing is 
    generic (
        N: integer
    );
    Port (
        clk: in std_logic;
        rst_n: in std_logic; --reset by zeroing the RGB vectors 
        demosaicing_en: in std_logic;
        
        -- OPTIONALLY, PASS DFFS DIAGONALLY IN THE WHOLE DEBAYER UNIT

        q00 : in std_logic_vector(7 downto 0);
        q01 : in std_logic_vector(7 downto 0);
        q02 : in std_logic_vector(7 downto 0);
        q10 : in std_logic_vector(7 downto 0);
        q11 : in std_logic_vector(7 downto 0);
        q12 : in std_logic_vector(7 downto 0);
        q20 : in std_logic_vector(7 downto 0);
        q21 : in std_logic_vector(7 downto 0);
        q22 : in std_logic_vector(7 downto 0);

        R: out std_logic_vector(7 downto 0);
        G: out std_logic_vector(7 downto 0);
        B: out std_logic_vector(7 downto 0)
    );
end Demosaicing; 

architecture Behavioral of Demosaicing is 
    signal row_counter : integer range 0 to N-1 := 0;
    signal pixel_counter : integer range 0 to N-1 := 0;
    
    type arr is array (2 downto 0) of std_logic_vector (7 downto 0);

begin

    process(clk) is
        variable L1 : arr; 
        variable L2 : arr; 
        variable L3 : arr;
        variable sum : unsigned(9 downto 0);
    begin
        if rising_edge(clk) then
            if (rst_n = '0') then
                R <= (others => '0');
                G <= (others => '0');
                B <= (others => '0');
                row_counter <= 0;
                pixel_counter <= 0;
            elsif (demosaicing_en = '1') then
           
                if (row_counter = 0) then
                    -- zero padding for first line
                    L1 := (others => (others => '0'));   
                    if (pixel_counter = 0) then     
                        -- zero padding for first column, use q01, q02, q11, q12 for computations !!!
                        L2 := (x"00", q11, q10);             
                        L3 := (x"00", q01, q00);           
                    elsif (pixel_counter = N-1) then
                        -- zero padding for last column, use q01, q02, q11, q12 for computations
                        L2 := (q12, q11, x"00");             
                        L3 := (q02, q01, x"00");  
                    else 
                        --use first 2 rows for computations
                        L2 := (q12, q11, q10);
                        L3 := (q02, q01, q00);        
                    end if; 
                elsif (row_counter = N-1) then
                    -- zero padding for last row
                    L3 := (others => (others => '0'));
                    if (pixel_counter = 0) then 
                        -- zero padding for first column, use q10, q11, q20, q21 for computations
                        L1 := (x"00", q21, q20);
                        L2 := (x"00", q11, q10); 
                    elsif (pixel_counter = N-1) then
                        -- zero padding for last column, use q11, q12, q21, q22 for computations
                        L1 := (q21, q20, x"00");
                        L2 := (q11, q10, x"00"); 
                    else 
                        -- use q10, q11, q12, q20, q21, q22 for computations
                        L1 := (q22, q21, q20);
                        L2 := (q12, q11, q10); 
                    end if;
                else
                    if (pixel_counter = 0) then 
                        -- zero padding for first column
                        L1 := (x"00", q21, q20); 
                        L2 := (x"00", q11, q10); 
                        L3 := (x"00", q01, q00); 
                    elsif (pixel_counter = N-1) then 
                        -- zero padding for last column 
                        L1 := (q22, q21, x"00"); 
                        L2 := (q12, q11, x"00");
                        L3 := (q02, q01, x"00");
                    else    
                        -- not an edge case, use the whole 3x3 grid
                        L1 := (q22, q21, q20);
                        L2 := (q12, q11, q10);
                        L3 := (q02, q01, q00);
                    end if;
                end if; 

                --------------------- RGB COMPUTATIONS ----------------------

                -- CASE 1 : odd row counter + odd pixel counter
                if (row_counter mod 2 = 1 and pixel_counter mod 2 = 1) then
                    sum := resize(unsigned(L2(0)), 10) + resize(unsigned(L2(2)), 10);
                    R   <= std_logic_vector(sum(8 downto 1)); -- Divide by 2, uses floor to approximate to an integer val
                    G   <= L2(1); 
                    sum := resize(unsigned(L1(1)), 10) + resize(unsigned(L3(1)), 10); 
                    B   <= std_logic_vector(sum(8 downto 1));

                -- CASE 2 : even row counter + odd pixel counter
                elsif (row_counter mod 2 = 0 and pixel_counter mod 2 = 1) then
                    sum := resize(unsigned(L1(0)), 10) + resize(unsigned(L1(2)), 10) + resize(unsigned(L3(0)), 10) + resize(unsigned(L3(2)), 10); 
                    R   <= std_logic_vector(sum(9 downto 2));
                    B   <= L2(1);
                    sum := resize(unsigned(L1(1)), 10) + resize(unsigned(L2(0)), 10) + resize(unsigned(L2(2)), 10) + resize(unsigned(L3(1)), 10);
                    G   <= std_logic_vector(sum(9 downto 2));

                -- CASE 3 : odd row counter + even pixel counter
                elsif (row_counter mod 2 = 1 and pixel_counter mod 2 = 0) then
                    R   <= L2(1); 
                    sum := resize(unsigned(L1(1)), 10) + resize(unsigned(L2(0)), 10) + 
                        resize(unsigned(L2(2)), 10) + resize(unsigned(L3(1)), 10);
                    G   <= std_logic_vector(sum(9 downto 2)); -- Divide by 4
                    sum := resize(unsigned(L1(0)), 10) + resize(unsigned(L1(2)), 10) + 
                        resize(unsigned(L3(0)), 10) + resize(unsigned(L3(2)), 10);
                    B   <= std_logic_vector(sum(9 downto 2));

                -- CASE 4 : even row counter + even pixel counter
                elsif (row_counter mod 2 = 0 and pixel_counter mod 2 = 0) then
                    sum := resize(unsigned(L1(1)), 10) + resize(unsigned(L3(1)), 10);
                    R   <= std_logic_vector(sum(8 downto 1));
                    G   <= L2(1); 
                    sum := resize(unsigned(L2(0)), 10) + resize(unsigned(L2(2)), 10);
                    B  <= std_logic_vector(sum(8 downto 1));
                end if;

                if (pixel_counter = N-1) then
                    pixel_counter <= 0; 
                    row_counter <= row_counter + 1;  -- Control Unit makes sure this runs exactly NxN times so that it's within bounds
                else 
                    pixel_counter <= pixel_counter + 1; 
                end if;
            else 
                R <= (others => '0');
                G <= (others => '0');
                B <= (others => '0');               
            end if;    
        end if;
    end process;

end Behavioral;