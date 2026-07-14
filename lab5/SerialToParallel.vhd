library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use ieee.numeric_std.ALL;

entity SerialToParallel is 
    generic (
        N: integer
    );
    Port ( 
        clk: in std_logic;
        rst: in std_logic;
        fifo_we: in std_logic;
        pixel: in std_logic_vector(7 downto 0);

        -- 3x3 DFF grid
        q00 : out std_logic_vector(7 downto 0);
        q01 : out std_logic_vector(7 downto 0);
        q02 : out std_logic_vector(7 downto 0);
        q10 : out std_logic_vector(7 downto 0);
        q11 : out std_logic_vector(7 downto 0);
        q12 : out std_logic_vector(7 downto 0);
        q20 : out std_logic_vector(7 downto 0);
        q21 : out std_logic_vector(7 downto 0);
        q22 : out std_logic_vector(7 downto 0)
    );
end SerialToParallel;

architecture Behavioral of SerialToParallel is

    COMPONENT fifo_generator_0
        PORT (
            clk : in std_logic;
            srst : in std_logic;
            din : in std_logic_vector(7 downto 0);
            wr_en : in std_logic;
            rd_en : in std_logic;
            dout : out std_logic_vector(7 downto 0);
            full : out std_logic;
            empty : out std_logic;
            data_count : out std_logic_vector(10 downto 0) -- length of data count is log2(N)
            );
    END COMPONENT;

    signal full0, full1, full2, empty0, empty1, empty2: std_logic;
    signal rd_en0, rd_en1, rd_en2: std_logic := '0';
    signal fifo0_out, fifo1_out, fifo2_out: std_logic_vector(7 downto 0);
    signal datacount0, datacount1, datacount2: std_logic_vector(10 downto 0) := (others => '0');

    type arr is array (2 downto 0) of std_logic_vector (7 downto 0);
    signal L0 : arr := (others => (others => '0'));
    signal L1 : arr := (others => (others => '0'));
    signal L2 : arr := (others => (others => '0'));

begin
    
    FIFO_0 : fifo_generator_0
    PORT MAP (
        clk => clk,
        srst => rst,
        din => pixel,
        wr_en => fifo_we, 
        rd_en => rd_en0,
        dout => fifo0_out,
        full => full0,
        empty => empty0,
        data_count => datacount0
    );
    
    FIFO_1 : fifo_generator_0
    PORT MAP (
        clk => clk,
        srst => rst,
        din => fifo0_out,
        wr_en => fifo_we,
        rd_en => rd_en1,
        dout => fifo1_out,
        full => full1,
        empty => empty1,
        data_count => datacount1
    );    

    FIFO_2 : fifo_generator_0
    PORT MAP (
        clk => clk,
        srst => rst,
        din => fifo1_out,
        wr_en => fifo_we,
        rd_en => rd_en2,
        dout => fifo2_out,
        full => full2,
        empty => empty2,
        data_count => datacount2
    );

    process(clk)
    begin
    if rising_edge(clk) then
        if (rst = '1') then
            rd_en0 <= '0';
            rd_en1 <= '0';
            rd_en2 <= '0'; 
            L0 <= (others => (others => '0'));
            L1 <= (others => (others => '0'));
            L2 <= (others => (others => '0'));
        elsif (fifo_we = '1') then
            -- FIFO physical size is at 1024 
            -- using datacount as a function of N allows for smaller images to be processed by the same fifo
            -- USE FWFT WITH DATA WIDTH 8 BITS AND ENABLE DATA_COUNT 
            if (to_integer(unsigned(datacount0)) = N-1) then 
                rd_en0 <= '1';
                rd_en1 <= '1';
                rd_en2 <= '1'; 
            end if;
            L0 <= L0(1 downto 0) & fifo0_out;
            L1 <= L1(1 downto 0) & fifo1_out;
            L2 <= L2(1 downto 0) & fifo2_out;
            q00 <= L0(0); q01 <= L0(1); q02 <= L0(2);   -- NOTE: in TB THE DFF GRID IS READY AT 2N + 3 +1(DELAY) CLOCK CYCLES AFTER THE FIRST PIXEL ARRIVES IN FIFO0
            q10 <= L1(0); q11 <= L1(1); q12 <= L1(2);
            q20 <= L2(0); q21 <= L2(1); q22 <= L2(2);  
        end if; 
    end if;
    end process;
    
end Behavioral;