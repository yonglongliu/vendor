#tfa.pyx
# wrapper class
from libc.stdlib cimport malloc, free

cimport ctfa

cdef class NXP_I2C:
	""" Tfa NXP_I2C HAL wrapper class"""
	cpdef public targetname
	""" interface target e.g. hid """
	cpdef public int slave
	""" i2c slave e.g. 0x34 """
	cpdef public int isopen
	""" slave is opened """
	cpdef public buffersize
	""" max burst size """
	cpdef public error
	""" last NXP_I2C error, 1 is ok"""
	cpdef public pending_write
	""" keep write data in split call with write/read """
	def __cinit__(self, mytarget="dummy", slave=0x36):
		self.error=ctfa.NXP_I2C_Open(mytarget)
		self.pending_write=None
		self.slave=slave
		self.targetname=mytarget
		self.isopen = self.error !=-1
		if self.isopen:
			self.buffersize=self.BufferSize()
	def Version(self):
		cpdef char *version
		version=''
		error = ctfa.NXP_I2C_Version(version)
		return version
	def Close(self):
		return ctfa.NXP_I2C_Close()
	def BufferSize(self):
		return ctfa.NXP_I2C_BufferSize()
	def Trace(self, state=True):
		ctfa.NXP_I2C_Trace(state)
	def Trace_file(self, filename=None):
		if filename:
			ctfa.NXP_I2C_Trace_file(filename)
		else:
			ctfa.NXP_I2C_Trace_file(<char*>0)
	def WriteReadString(self, i2c_string, num_read_bytes=None):
		""" input is '0x68 0x71 0x34 0x21' [numreadbytes] or
			  traceline: 'I2C w [  4]: 0x68 0x71 0xaa 0x55' """
		#print '\nWriteReadString',i2c_string,num_read_bytes

		if i2c_string[:2]=='0x':
			#dev.WriteRead('0x68 0x71 0x34 0x21')
			argtype=0 # hexstring args
			write_data = i2c_string
		elif i2c_string[:6]=='I2C w ': # write only
			#dev.WriteRead('I2C w [  4]: 0x68 0x71 0xaa 0x55')
			argtype=1 # traceline string args
			write_data=i2c_string.split(': ')[1]  
		elif i2c_string[:6]=='I2C W ': # write part of write /read
			#dev.WriteRead('I2C w [  4]: 0x68 0x71 0xaa 0x55')
			argtype=2 # traceline string args
			self.pending_write=i2c_string.split(': ')[1] 
			return '' 
		elif i2c_string[:6]=='I2C R ': # read part of write /read
			if not self.pending_write:
				print "Missing I2C W part of this write/read transaction"
				return ''
			write_data = self.pending_write
			self.pending_write = None
			#only calculate read size
			read_bytes = i2c_string.split(': ')[1]
			read_bytes = bytearray.fromhex(write_data.replace('0x',''))
			num_read_bytes = len(read_bytes)
		write_bytes = bytearray.fromhex(write_data.replace('0x',''))
		slave=write_bytes[0]/2
		write_bytes = write_bytes[1:]		
		
		if not num_read_bytes:
			num_read_bytes=0

		read_buffer=bytearray(num_read_bytes)
		num_write_bytes = len(write_bytes)
			 
		self.error = ctfa.NXP_I2C_WriteRead(slave*2, num_write_bytes, write_bytes,
											  num_read_bytes, read_buffer )
	
	def WriteRead(self, slave, write_data=None,num_read_bytes=None,  other_num_read_bytes=None):
		cpdef unsigned short word
		argtype=None
		#print slave,write_data,num_read_bytes,other_num_read_bytes
		if type(slave)==int and type(write_data)==str:
			#dev.WriteRead(0x34, '0x71 0x12 0x34')
			argtype=0 #asci args
			write_bytes = bytearray.fromhex(write_data.replace('0x','')) 
		elif type(slave)==int and type(write_data)==int:
			#dev.WriteRead(0x34, 0x71, 0x5678)
			argtype=1 #binary args
			word=write_data
			if num_read_bytes:
				write_bytes = bytearray(3)
				write_bytes[0] = write_data
				write_bytes[1] = num_read_bytes&0xff
				write_bytes[2] = num_read_bytes>>8
			else:
				write_bytes = bytearray(1)
				write_bytes[0] = write_data
			num_read_bytes=other_num_read_bytes
		elif type(slave)==str:
			return self.WriteReadString(slave, num_read_bytes)
		if not num_read_bytes:
			num_read_bytes=0

		read_buffer=bytearray(num_read_bytes)
		num_write_bytes = len(write_bytes)
			 
		self.error = ctfa.NXP_I2C_WriteRead(slave*2, num_write_bytes, write_bytes,
											  num_read_bytes, read_buffer )
		read_string='!error'
		if self.error==1:#=ok
			read_string='' #'0x%x'%(slave*2)
			for i in range(num_read_bytes):
				read_string += ' 0x%x'%read_buffer[i]
		if argtype==0:
			return read_string[1:]
	def SetPin(self,pin,value):
		return ctfa.NXP_I2C_SetPin(pin,value)
	def GetPin(self,pin):
		return ctfa.NXP_I2C_GetPin(pin)
	def regWrite16(self,slave,reg,write_data):
		read_buffer=bytearray(1)
		write_bytes = bytearray(4)
		write_bytes[0] = reg
		write_bytes[1] = write_data>>8
		write_bytes[2] = 0xff & write_data
		self.error = ctfa.NXP_I2C_WriteRead(slave*2, 3, write_bytes,0, read_buffer )
	def regRead16(self,slave,reg):
		read_buffer=bytearray(2)
		write_bytes = bytearray(1)
		write_bytes[0] = reg
		self.error = ctfa.NXP_I2C_WriteRead(slave*2, 1, write_bytes,2, read_buffer )
		#print "%x %x"%(read_buffer[1],read_buffer[0])
		if self.error==1:#=ok
			value= read_buffer[0]<<8 | read_buffer[1]
		else:
			value=None
		return value
	def regWrite8(self,slave,reg,write_data):
		read_buffer=bytearray(1)
		write_bytes = bytearray(3)
		write_bytes[0] = reg
		write_bytes[1] = write_data
		self.error = ctfa.NXP_I2C_WriteRead(slave*2, 2, write_bytes,0, read_buffer )
	def regRead8(self,slave,reg):
		read_buffer=bytearray(1)
		write_bytes = bytearray(1)
		write_bytes[0] = reg
		self.error = ctfa.NXP_I2C_WriteRead(slave*2, 1, write_bytes,1, read_buffer )
		#print "%x %x"%(read_buffer[1],read_buffer[0])
		if self.error==1:#=ok
			value= read_buffer[0]
		else:
			value=None
		return value
cdef class Tfa:
	""" Tfa wrapper class"""
	cpdef public int slave
	""" i2c slave e.g. 0x34 """
	cpdef public int revid
	""" tfa device type e.g. 0x88 """
	cpdef public targetname
	""" interface target e.g. hid """
	cpdef int handle
	cpdef public int isopen
	cpdef public int bftrace
 ##   cdef ctfa.Tfa* _c_tfa
	def __cinit__(self, mytarget="dummy", slave=0x34, revid=0x88):
		error=0
		self.slave=slave
		self.revid=revid
		self.targetname=mytarget
		self.handle=-1
		self.isopen=False
		self.bftrace=False
		if mytarget:
			error=self.target(mytarget)
			
	  #  self._c_tfa = ctfa.tfa_new() 
	   # if self._c_tfa is NULL:
		#	raise MemoryError()

   # def __dealloc__(self):
		#if self._c_tfa is not NULL:
		 #   ctfa.tfa_free(self._c_tfa)
	cpdef open(self):
		if self.isopen:
			return
		if self.handle<0:
			"not probed"
			return
		if ctfa.tfa98xx_open(self.handle)==0:
			self.isopen=True
	cpdef close(self):
		if not self.isopen:
			"not open"
			return
		ctfa.tfa98xx_close(self.handle)
		self.isopen=False
	cpdef probe(self, slave):
		cdef int *phandle
		phandle = &self.handle
		if slave==None:
			slave=self.slave
		ctfa.tfa_probe(slave*2, phandle)
	cpdef verbose(self,level=1):
		ctfa.tfa_cnt_verbose(level)
	cpdef trace(self,state=1):
		ctfa.NXP_I2C_Trace(state)
	cpdef trace_file(self,file):
		self.trace()
		ctfa.NXP_I2C_Trace_file(file)
	cpdef wr(self,reg, value):
		wasopen = self.isopen
		if not wasopen:
			self.open()
		error = ctfa.tfa98xx_write_register16(self.handle, reg,value)
		if not wasopen:
			self.close()
	cpdef rd(self,reg):
		cdef unsigned short *pvalue
		cpdef unsigned short value
		wasopen = self.isopen
		if not wasopen:
			self.open()
		error = ctfa.tfa98xx_read_register16(self.handle, reg,&value)
		if not wasopen:
			self.close()
		#value = *pvalue
		return value
	cpdef wrbf(self,_name, value):
		name="niks"
		name=_name
		#cdef unsigned short bf
		bf=self.bfenum(name,self.revid)
		if bf==0xffff:
			print name,"is unknown"
			return
		wasopen = self.isopen
		if not wasopen:
			self.open()
		error = ctfa.tfa_set_bf(self.handle, bf,value)
		if not wasopen:
			self.close()
		if self.bftrace:
			print "%s=%d"%(name,value)
	cpdef rdbf(self,name):
		bf=self.bfenum(name,self.revid)
		if bf==0xffff:
			print name,"is unknown"
			return
		wasopen = self.isopen
		if not wasopen:
			self.open()
		value = ctfa.tfa_get_bf(self.handle, bf)
		if not wasopen:
			self.close()
		if self.bftrace:
			print "%s:%d"%(name,value)
		#value = *pvalue
		return value
	cpdef wrxmem(self,address, value):
		cpdef int cvalue
		cvalue=value
		wasopen = self.isopen
		if not wasopen:
			self.open()
		error = ctfa.tfa98xx_dsp_write_mem_word(self.handle,
					<unsigned short >address, 
									 cvalue, 1)
		if not wasopen:
			self.close()
	cpdef rdxmem(self,address):
		cpdef int cvalue
		cdef unsigned short *pvalue
		cpdef unsigned short value
		wasopen = self.isopen
		if not wasopen:
			self.open()
		error = ctfa.tfa98xx_dsp_read_mem(self.handle,
				   address,
				   1, &cvalue)
		if not wasopen:
			self.close()
		#value = *pvalue
		return cvalue

	
	cpdef start(self,profile=0, int vstep=0):
		""" start device """
		cdef int vsteps[2]
		vsteps[0]=vsteps[1]=vstep#,vstep] #left=rigth
		return ctfa.tfa_start(profile, vsteps)
		
	cpdef show(self):
		""" dump Tfa struct values """
		print "target:%s, slave:0x%02x, revid:0x%02x\n"%(self.targetname,self.slave, self.revid)
		
	cpdef target(self, char *dev):
		return ctfa.NXP_I2C_Open(<char*>dev)
		#if not ctfa.lxScriboRegister(<char*>dev):
		 #   raise MemoryError()
	cpdef container(self, char *filename):
		cdef unsigned char slave
		dev=0
		if ctfa.tfa98xx_cnt_loadfile(<char*>filename,<int> 0)==0:
			ctfa.tfa_probe_all(ctfa.tfa98xx_get_cnt())
#
#this needs climain in the library	  
#	 cpdef climax(self,char *cmdline):
#		 print cmdline
#		 args = cmdline.split()
#		 argc = len(args)
#		 cdef char **c_argv
#		 args.insert(0,'cython')
#		 c_argv = <char**>malloc(sizeof(char*) * len(args)) # + try/finally and free!!
#		 #forece close first, climaxkeep dev open
#		 self.isopen=1
#		 self.close()
#		 error = ctfa.climain(len(args), c_argv)
#		 free(c_argv)
#		 return error

	cpdef bfenum(self, name,rev=None,family=2):
		if not rev:
			rev = self.revid
		if rev != 0x88:
			family=1
		return ctfa.tfaContBfEnum(<const char *>name, <int> family, <unsigned short> rev)
	cpdef bfname(self,bfnum,rev):
		name=''
		cdef unsigned short num=bfnum
		cdef unsigned short revid=rev
		family=2
		name= ctfa.tfaContBfName(<unsigned short>num, <unsigned short>revid)
		if name=='Unknown bitfield enum':
			name = None
		return name

	
