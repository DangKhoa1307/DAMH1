#define Spi_Ethernet_HALFDUPLEX     0
#define Spi_Ethernet_FULLDUPLEX     1

/************************************************************
 * ROM constant strings
 */
const unsigned char httpHeader[] = "HTTP/1.1 200 OK\nContent-type: " ;  // HTTP header
const unsigned char httpMimeTypeHTML[] = "text/html\n\n" ;              // HTML MIME type
const unsigned char httpMimeTypeScript[] = "text/plain\n\n" ;           // TEXT MIME type
unsigned char httpMethod[] = "GET /";
unsigned char sec, mnt, hr, day, date, mn, year, cnt, addrh, addrl ;
unsigned int adv,addee,updt;
char text[10];
/*
 * web page, splited into 2 parts :
 * when coming short of ROM, fragmented data is handled more efficiently by linker
 *
 * this HTML page calls the boards to get its status, and builds itself with javascript
 */
const   char    *indexPage =                   // Change the IP address of the page to be refreshed ,tram5.dnsalias.net
"<meta http-equiv=\"refresh\" content=\"3;url=http://192.168.1.7\">\
<HTML><center>\<body bgcolor=#CC9966 >\
<title> do an dieu khien thiet bi</title> \
<center>\
<h1 style=\"color:#FF0000\">tr&#432;&#7901;ng cao &#273;&#7859;ng k&#7929; thu&#7853;t cao th&#7855;ng</h1>\
<h1 style=\"color:#FF0000\">&#272;&#7890; &#193;N T&#7888;T NGHI&#7878;P</h1/>\
<h1 style=\"color:#FF0000\">&#272;&#7872; T&#192;I: &#272;I&#7872;U KHI&#7874;N THI&#7870;T B&#7882; QUA INTERNET</H1>\
<h1 style=\"color:#FF0000\">GVHD: ThS V&Otilde; XU&Acirc;N NAM </h1>\
<h1 style=\"color:#8DEEEE\">SVTH: HU&#7922;NH V&#258;N TI&Ecirc;N <BR> &#272;O&Agrave;N NG&#7884;C S&#416;N <BR> C&#272; &#272;TVT10C</h1>\
<script src=/s></script>\
<table><tr><td valign=top><table border=1 style=\"font-size:20px ;font-family: terminal ;\">\
<a href=/><center>Reload</center></a>\
" ;
const   char    *indexPage2 =  "</table></td><td>\
<table border=1 style=\"font-size:20px ;font-family: terminal ;\">\
<tr><th colspan=3>BANG DIEU KHIEN</th></tr>\
<script>\
var str,i;\
str=\"\";\
for(i=0;i<4;i++)\
{str+=\"<tr><td bgcolor=#00CC00> TB \"+i+\"</td>\";\
if(PORTD&(1<<i)){str+=\"<td bgcolor= #00FF00> ON\";}\
else {str+=\"<td bgcolor= #FF0000> OFF\";}\
str+=\"</td><td><a href=/t\"+i+\"> ON/OFF \"+i+\"</a></td></tr>\";}\
document.write(str) ;\
</script>\
</table></td></tr></table></center>\
<center>This is HTTP request #<script>document.write(REQ)</script></center></BODY></center></HTML>\
" ;


/***********************************
 * RAM variables
 */

unsigned char   myMacAddr[6] = {0x00, 0x14, 0xA5, 0x76, 0x19, 0x3f} ;   // my MAC address
unsigned char   myIpAddr[4]  = {192, 168,   1, 7 } ;                   // my IP address
unsigned char   gwIpAddr[4]  = {192, 168,   1,  5 } ;                   // gateway (router) IP address
unsigned char     ipMask[4]    = {255, 255, 255,  0 } ;                   // network mask (for example : 255.255.255.0)
unsigned char     dnsIpAddr[4] = {192, 168,   1,  5 } ;                     // DNS server IP address

unsigned char   getRequest[15] ;                                        // HTTP request buffer
unsigned char   dyna[31] ;                                              // buffer for dynamic response
unsigned long   httpCounter = 0 ;                                      // counter of HTTP requests

/*******************************************
 * functions
 */

/*
 * put the constant string pointed to by s to the ENC transmit buffer
 */
unsigned int    putConstString(const char *s)
        {
        unsigned int ctr = 0 ;

        while(*s)
                {
                Spi_Ethernet_putByte(*s++) ;
                ctr++ ;
                }
        return(ctr) ;
        }

/*
 * put the string pointed to by s to the ENC transmit buffer
 */
unsigned int    putString(char *s)
        {
        unsigned int ctr = 0 ;

        while(*s)
                {
                Spi_Ethernet_putByte(*s++) ;
                ctr++ ;
                }
        return(ctr) ;
        }

unsigned int    Spi_Ethernet_UserTCP(unsigned char *remoteHost, unsigned int remotePort, unsigned int localPort, unsigned int reqLength)
  {
        unsigned int    len = 0 ;                   // my reply length
        unsigned int    i ;                         // general purpose integer

        if(localPort != 80)                         // I listen only to web request on port 80
                {
                return(0) ;
                }

        // get 10 first bytes only of the request, the rest does not matter here
        for(i = 0 ; i < 10 ; i++)
                {
                getRequest[i] = Spi_Ethernet_getByte() ;
                }
        getRequest[i] = 0 ;

        if(memcmp(getRequest, httpMethod, 5))       // only GET method is supported here
                {
                return(0) ;
                }

        httpCounter++ ;                             // one more request done

        if(getRequest[5] == 's')                    // if request path name starts with s, store dynamic data in transmit buffer
                {
                // the text string replied by this request can be interpreted as javascript statements
                // by browsers

                len = putConstString(httpHeader) ;              // HTTP header
                len += putConstString(httpMimeTypeScript) ;     // with text MIME type


                // add AN3 value to reply
                ByteToStr(ADC_Read(0)/2.046, dyna) ;
                len += putConstString("var AN0=") ;
                len += putString(dyna) ;
                len += putConstString(";") ;
                // add PORTD value (LEDs) to reply
                len += putConstString("var PORTD=") ;
                intToStr(LATD, dyna) ;
                len += putString(dyna) ;
                len += putConstString(";") ;

                // add HTTP requests counter to reply
                intToStr(httpCounter, dyna) ;
                len += putConstString("var REQ=") ;
                len += putString(dyna) ;
                len += putConstString(";") ;
                }
        else if(getRequest[5] == 't')                           // if request path name starts with t, toggle PORTD (LED) bit number that comes after
                {
                unsigned char   bitMask = 0 ;                  // for bit mask

                if(isdigit(getRequest[6]))                      // if 0 <= bit number <= 9, bits 8 & 9 does not exist but does not matter
                        {
                        bitMask = getRequest[6] - '0' ;         // convert ASCII to integer
                        bitMask = 1 << bitMask ;                // create bit mask
                        LATD ^= bitMask ;                     // toggle PORTD with xor operator
                        }
                }

        if(len == 0)                                            // what do to by default
                {
                len =  putConstString(httpHeader) ;             // HTTP header
                len += putConstString(httpMimeTypeHTML) ;       // with HTML MIME type
                len += putConstString(indexPage) ;              // HTML page first part
                len += putConstString(indexPage2) ;             // HTML page second part
                }

        return(len) ;                                           // return to the library with the number of bytes to transmit
        }


unsigned int    Spi_Ethernet_UserUDP(unsigned char *remoteHost, unsigned int remotePort, unsigned int destPort, unsigned int reqLength)
        {
          char index = 0 ;
          unsigned int addt,addpc,len,adv;
          //read udp request
          while(reqLength--)
                {
                text[index++] = SPI_Ethernet_getByte() ;
                }
          text [index] = 0 ;
          index = 0;
          switch(text[0])
          {
              case 'b': {
                   adv=ADC_Read(0);
                   dyna[2]=adv;
                   dyna[3]=adv>>8;
                   Spi_Ethernet_putByte(49) ;
                   Spi_Ethernet_putByte(dyna[2]) ;
                   Spi_Ethernet_putByte(dyna[3]) ;
                   return(3);
                   break;
                   }
              case 's': {
                     updt=(text[1] << 8) + text[2];
                     Spi_Ethernet_putByte('d');
                     Spi_Ethernet_putByte('o');
                     Spi_Ethernet_putByte('n');
                     Spi_Ethernet_putByte('e');
                     return(4);
                     break;
                     }

              case 'u' : {
                     //addt=1023;
                     addpc=(text[1] << 8) + text[2];
                     if (addee < addpc) addt=32768-(addpc-addee);
                     else addt=addee-addpc;
                     if (addt > 1024) addt = 1024;
                     len=addt+8;
                     Spi_Ethernet_putByte(221);
                     Spi_Ethernet_putByte(addee);
                     Spi_Ethernet_putByte(addee>>8);
                     Spi_Ethernet_putByte(addt);
                     Spi_Ethernet_putByte(addt>>8);
                     Spi_Ethernet_putByte(0);
                     Spi_Ethernet_putByte(0);
                     Spi_Ethernet_putByte(0);
                     break;
                     }
              case 'v' : {

                     adv=ADC_Read(1);
                     dyna[0]=addee;
                     dyna[1]=addee>>8;
                     Spi_Ethernet_putByte(51) ;
                     Spi_Ethernet_putByte(cnt) ;
                     Spi_Ethernet_putByte(date) ;
                     Spi_Ethernet_putByte(mn) ;
                     Spi_Ethernet_putByte(year) ;
                     Spi_Ethernet_putByte(hr) ;
                     Spi_Ethernet_putByte(mnt) ;
                     Spi_Ethernet_putByte(sec) ;
                     Spi_Ethernet_putByte(addee) ;
                     Spi_Ethernet_putByte(addee>>8) ;
                     return(10) ;       // 10 bytes to transmit
                     break;
                     }
              default:   {
                     // should never enter here
                     }
          }
        }

/* Interrupts */
void interrupt() {
     cnt++;
     LATB.F3 = !LATB.F3;
     INTCON.INT0IF = 0;       //INT0 Flag reset

}
/*
 * main entry
 */
 unsigned int dox;
 char degree[5];
void    main()
        {

        ADCON1 = 0x0E ;         // ADC convertors will be used

//Interrupt Configuration
        INTCON.GIE = 1; //High Priority Enable
        RCON.IPEN = 1;  //High Priority Enable
        INTCON2.INTEDG0 = 1; //Ext Int0 on rising edge
        INTCON.INT0IE = 1;   //INT0 enable
        INTCON.INT0IF = 0; // INT0 Flag reset
        TRISA = 0x07 ;          // set PORTA as input for ADC

        PORTD = 0 ;
        TRISD = 0 ;             // set PORTD as output
        cnt=0;                  // update counter
        updt=60;                // update time

        /*
         * starts ENC28J60 with :
         * reset bit on RC0
         * CS bit on RC1
         * my MAC & IP address
         * full duplex
         */
        Spi_Init();
        Spi_Ethernet_Init(&PORTE, 0, &PORTE, 1, myMacAddr, myIpAddr, Spi_Ethernet_FULLDUPLEX) ;

        while(1)                        // do forever
                {
        Spi_Ethernet_doPacket() ;   // process incoming Ethernet packets

                }
        }