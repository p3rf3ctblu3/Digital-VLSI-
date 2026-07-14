library IEEE;
use IEEE.STD_LOGIC_1164.ALL;use IEEE.NUMERIC_STD.ALL;
use std.textio.all;      
use ieee.std_logic_textio.all;

entity DebayeringFilter_TB is
end DebayeringFilter_TB;

architecture Behavioral of DebayeringFilter_TB is
    
    component DebayeringFilter is
        generic (
            N: integer := 32
        );
        Port ( 
            clk, rst_n: in std_logic;
            new_image, valid_in: in std_logic;
            pixel: in std_logic_vector(7 downto 0);
            image_finished, valid_out: out std_logic;
            R, G, B: out std_logic_vector (7 downto 0)
        );
    end component;
   
    constant N : integer := 32;
    constant CLK_PERIOD : time := 10 ns;

    -- UUT Signals
    signal clk            : std_logic;
    signal rst_n          : std_logic;

    signal pixel : std_logic_vector (7 downto 0);
    signal new_image, valid_in : std_logic;
    signal q00, q01, q02, q10, q11, q12, q20, q21, q22: std_logic_vector(7 downto 0) := (others => '0');
    signal image_finished, valid_out : std_logic;   
    signal R,G,B : std_logic_vector (7 downto 0);

begin

    clk_process: process
    begin
        clk <= '0';
        wait for CLK_PERIOD/2;
        clk <= '1';
        wait for CLK_PERIOD/2;
    end process;

    DUT: DebayeringFilter port map 
    (
        clk => clk, 
        rst_n => rst_n,
        new_image => new_image,
        valid_in => valid_in,
        pixel => pixel,
        image_finished => image_finished,
        valid_out => valid_out,
        R => R,
        G => G, 
        B => B
    );

    TEST: 
        process
            file     text_file : text open read_mode is "32x32.txt";
            variable text_line : line;
            variable temp_int  : integer;
        begin
        
            -- set generic = 4 to test
            
--            rst_n <= '0';
--            wait for CLK_PERIOD * 5;

--            rst_n <= '1';   
--            new_image <= '1'; 
--            valid_in <= '1';
            
--            pixel <= "00000001"; 
--            wait for CLK_PERIOD;

--            new_image <= '0';
--            pixel <= "00000010"; 
--            wait for CLK_PERIOD;
   
--            pixel <= "00000011";
--            wait for CLK_PERIOD; 

--            pixel <= "00000100";
--            wait for CLK_PERIOD; 

--            pixel <= "00000101";
--            wait for CLK_PERIOD; 

--            pixel <= "00000110";
--            wait for CLK_PERIOD; 

--            pixel <= "00000111";
--            wait for CLK_PERIOD; 

--            pixel <= "00001000";
--            wait for CLK_PERIOD; 

--            pixel <= "00001001";
--            wait for CLK_PERIOD; 

--            pixel <= "00001010";
--            wait for CLK_PERIOD; 

--            pixel <= "00001011";
--            wait for CLK_PERIOD; 

--            pixel <= "00001100";
--            wait for CLK_PERIOD; 

--            pixel <= "00001101";
--            wait for CLK_PERIOD; 

--            pixel <= "00001110";
--            wait for CLK_PERIOD; 

--            pixel <= "00001111";
--            wait for CLK_PERIOD; 

--            pixel <= "00010000";
--            wait for CLK_PERIOD; 
            
            
            ----------------------------------------------- 32 x 32 TEST ------------------------------------------------
            
            rst_n <= '0';
            wait for CLK_PERIOD * 5;

            rst_n <= '1';   
            new_image <= '1'; 
            valid_in <= '1';
            
            while not endfile(text_file) loop
                readline(text_file, text_line);  
                read(text_line, temp_int);       
                
                pixel <= std_logic_vector(to_unsigned(temp_int, 8));
            
                wait until rising_edge(clk);     -- Προώθηση στο επόμενο pixel στον επόμενο κύκλο
            end loop;
            
            valid_in <= '0';

        wait;
        end process;

end Behavioral;